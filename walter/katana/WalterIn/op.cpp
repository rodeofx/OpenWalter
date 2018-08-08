#include "walterUSDOp.h"

#include <boost/shared_ptr.hpp>

#include <FnAttribute/FnAttribute.h>
#include <FnAttribute/FnGroupBuilder.h>

#include <pystring/pystring.h>

#include <FnPluginSystem/FnPlugin.h>
#include <FnGeolib/op/FnGeolibOp.h>
#include <FnGeolib/op/FnGeolibCookInterface.h>
#include <FnGeolib/util/AttributeKeyedCache.h>
#include <FnGeolibServices/FnGeolibCookInterfaceUtilsService.h>

#include "AbcCook.h"
#include "ArrayPropUtils.h"
#include "ArbitraryGeomParamUtils.h"
#include "ScalarPropUtils.h"
#include "PathUtil.h"

#include <Alembic/AbcCoreFactory/All.h>
#include "IWalterExpressionsSchema.h"

#include <hdf5.h>  // for H5dont_atexit

#include <boost/algorithm/string.hpp>
#include <unordered_map>
#include <unordered_set>


namespace { // anonymous

/**
 * @brief Split long string that contains the list of Alembic layers into
 * vector. It considers hidden layers.
 *
 * @param iFileNameStr The string that contains the list of Alembic layers.
 * @param oArchives Output vector.
 */
void splitArchives(
    const std::string& iFileNameStr,
    std::vector<std::string>& oArchives)
{
    std::vector<std::string> archives;
    boost::split(archives, iFileNameStr, boost::is_any_of(":"));

    // We should ignore the archives if they start with '!' symbol
    for (const auto& archive : archives)
    {
        if (!archive.empty() && archive[0] != '^')
        {
            oArchives.push_back(archive);
        }
    }
}

Alembic::Abc::IArchive getArchive(
        Alembic::AbcCoreFactory::IFactory& iFactory,
        const std::string& iFileNames,
        Alembic::AbcCoreFactory::IFactory::CoreType& oType)
{
    std::vector<std::string> archives;
    splitArchives(iFileNames, archives);

    Alembic::AbcCoreFactory::IFactory factory;

    if (archives.size() == 0)
    {
        return Alembic::Abc::IArchive();
    }
    else if (archives.size() == 1)
    {
        return iFactory.getArchive(archives[0], oType);
    }

    return iFactory.getArchive(archives, oType);
}

class PathMap
{
public:
    PathMap()
    {
    };

    ~PathMap()
    {
    }

    WalterIn::AbcCookPtr get(const std::string & iFullName,
                              Alembic::Abc::IObject & iParent,
                              const std::string & iName)
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        std::map<std::string, WalterIn::AbcCookPtr>::iterator it =
            pathMap.find(iFullName);
        if (it != pathMap.end())
        {
            return it->second;
        }

        WalterIn::AbcCookPtr cookPtr(new WalterIn::AbcCook());
        cookPtr->objPtr =
            Alembic::Abc::IObjectPtr(new Alembic::Abc::IObject(iParent, iName));
        pathMap[iFullName] = cookPtr;
        return cookPtr;
    }

private:
    std::map<std::string, WalterIn::AbcCookPtr> pathMap;
    boost::mutex mutex;
};

typedef boost::shared_ptr< PathMap > PathMapPtr;

// Building following structure:
// {"target": {"object": "shader"}},
// where target is shader/displacement/attribute, object is "/.*", shader is
// "marble1"
typedef std::map<WalterCommon::Expression, std::string> ObjectToShader;
typedef std::unordered_map<std::string, ObjectToShader> TargetToAssignments;
typedef boost::shared_ptr<TargetToAssignments> AssignmentsPtr;

// Building following structure:
// ({"target": "shader"})
typedef std::pair<std::string, std::string> Material;
typedef std::unordered_set<Material, boost::hash<Material>> ShaderSet;
typedef boost::shared_ptr<ShaderSet> ShaderSetPtr;

// Keeps all the atributes of Alembic
typedef FnAttribute::GroupAttribute GroupAttribute;
// The first is the standard attribute, the second is the user attribute.
typedef std::pair<GroupAttribute, GroupAttribute> AttributePair;
typedef std::unordered_map<std::string, AttributePair> Attributes;
typedef boost::shared_ptr<Attributes> AttributesPtr;

// Some of the KTOA attributes don't match to the Arnold attributes. We receive
// Arnold attributes, and we need to make KTOA understanding them. The only way
// to do it is remapping them.
static const WalterIn::NameMap arnoldAttributes = {
    {"subdiv_iterations", "iterations"},
    {"disp_zero_value", "zero_value"},
    {"primary_visibility", "sidedness.AI_RAY_CAMERA"},
    {"casts_shadows", "sidedness.AI_RAY_SHADOW"},
    {"visible_in_reflections", "sidedness.AI_RAY_REFLECTED"},
    {"visible_in_refractions", "sidedness.AI_RAY_REFRACTED"},
    {"visible_in_diffuse", "sidedness.AI_RAY_DIFFUSE"},
    {"visible_in_glossy", "sidedness.AI_RAY_GLOSSY"}};

// Read assignments from Alembic's root and save it to oAssignments
bool readAssignments(
        Alembic::Abc::IObject iTop,
        AssignmentsPtr oAssignments)
{
    static const char* keys[] = {"shader", "displacement", "attribute"};

    if (!oAssignments)
    {
        return false;
    }

    oAssignments->clear();

    IWalterExpressionsSchema expressions;
    if (!hasExpressions(iTop, expressions) || !expressions.valid())
    {
        return false;
    }

    for (size_t i=0; i<expressions.getNumProperties(); i++)
    {
        const std::string& objName = expressions.getExpression(i);
        if (objName.empty())
        {
            continue;
        }

        IWalterLayersSchema layers = expressions.getLayerSchema(i);
        if (!layers.valid())
        {
            continue;
        }

        for (size_t j=0; j<layers.getNumTargets(); j++)
        {
            // Skip all the render layers except the default one.
            const std::string& layer = layers.getTargetName(j);
            if (!layer.empty() && layer != "defaultRenderLayer")
            {
                continue;
            }

            BOOST_FOREACH (auto key, keys)
            {
                std::string value = layers.getShaderName(layer, key);
                if (!value.empty())
                {
                    (*oAssignments)[key].emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(objName),
                        std::forward_as_tuple(value));
                }
            }
        }
    }

    return true;
}

// Read attributes from Alembic and save it to oAttributes
bool readAttributes(
        Alembic::Abc::IObject iTop,
        AttributesPtr oAttributes)
{
    Alembic::Abc::IObject materials(iTop, "materials");

    size_t numChildren = materials.getNumChildren();

    for (size_t i = 0; i < numChildren; i++)
    {
        const Alembic::AbcCoreAbstract::ObjectHeader& header =
            materials.getChildHeader(i);
        const std::string& name = header.getName();
        Alembic::AbcMaterial::IMaterial material(materials, name);
        Alembic::AbcMaterial::IMaterialSchema& schema = material.getSchema();

        std::string shader;
        if (!schema.getShader("arnold", "attribute", shader) ||
            shader != "walterOverride")
        {
            // We only need walterOverride.
            continue;
        }

        Alembic::Abc::ICompoundProperty parameters =
            schema.getShaderParameters("arnold", "attribute");

        // Convert Alembic parameters to Katana.
        auto cookPtr = boost::make_shared<WalterIn::AbcCook>();
        FnAttribute::GroupBuilder parametersGroup;
        processUserProperties(
                cookPtr,
                parameters,
                parametersGroup,
                "",
                &arnoldAttributes);

        // Save user parameters.
        FnAttribute::GroupBuilder userParametersGroup;
        processArnoldUserProperties(
                cookPtr,
                parameters,
                userParametersGroup,
                "");

        // Save them to use in the future.
        // TODO: emplace?
        (*oAttributes)[name] =
            std::make_pair(
                    parametersGroup.build(),
                    userParametersGroup.build());
    }
}

// Returns the shader name for specified object, layer and target.
std::string getShaderAssignment(
    const std::string& iObjectName,
    const AssignmentsPtr& iAssignments,
    const std::string& iTarget)
{
    // target is shader/displacement/attribute
    auto target = iAssignments->find(iTarget);
    if (target == iAssignments->end())
    {
        return "";
    }

    const std::string* layers = WalterCommon::resolveAssignment<std::string>(
        iObjectName, target->second);

    if (!layers)
    {
        return "";
    }

    // Found. Use the line before '.' character.
    const std::string& result = *layers;
    return result.substr(0, result.find('.'));
}

// Analyze the assignments and make a list of materials with both surface and
// displacement shaders.
bool produceShaderSets(
        const AssignmentsPtr& iAssignments,
        ShaderSetPtr oShaderSet)
{
    // Save all the surface and displacement shaders.
    auto surfaceIt = iAssignments->find("shader");
    auto displacementIt = iAssignments->find("displacement");

    if (surfaceIt != iAssignments->end() &&
        displacementIt != iAssignments->end())
    {
        // Iterate all the shaders to know if the combinations are possible.
        for (const auto& surface : surfaceIt->second)
        {
            for (const auto& displacement : displacementIt->second)
            {
                const WalterCommon::Expression& surfExp = surface.first;
                const WalterCommon::Expression& dispExp = displacement.first;

                // The combination is possible if the expressions are the same
                // and if it's possible to resolve the one with another as an
                // expression.
                if (!surfExp.isRegex() && !dispExp.isRegex())
                {
                    const std::string& surfStr = surfExp.getExpression();
                    const std::string& dispStr = dispExp.getExpression();

                    // Both are direct paths.
                    if (!surfExp.isParentOf(dispStr, nullptr) &&
                        !dispExp.isParentOf(surfStr, nullptr))
                    {
                        // Both are not a parent of each others.
                        continue;
                    }
                }
                else if (
                    !surfExp.matchesPath(dispExp) &&
                    !dispExp.matchesPath(surfExp))
                {
                    continue;
                }

                oShaderSet->emplace(surface.second, displacement.second);
            }
        }
    }

    return true;
}

// Add a default set of Arnold attributes to the node.
void setArnoldDefaults(Foundry::Katana::GeolibCookInterface &interface)
{
    // We have only smoothing for now.
    // We need smoothing because Arnold doesn't read normals it this attribute
    // is off.
    interface.setAttr(
            "arnoldStatements.smoothing",
            FnAttribute::IntAttribute(1));
}

struct ArchiveAndFriends
{
    ArchiveAndFriends(Alembic::Abc::IArchive & iArchive,
                      Alembic::AbcCoreFactory::IFactory::CoreType iCoreType) :
        pathMap(new PathMap()),
        assignments(new TargetToAssignments()),
        attributes(boost::make_shared<Attributes>()),
        shaderSet(new ShaderSet())
    {
        objArchive = iArchive.getTop();
        coreType = iCoreType;
        // Read and initialize assignments
        readAssignments(objArchive, assignments);
        readAttributes(objArchive, attributes);
        produceShaderSets(assignments, shaderSet);
    }

    Alembic::Abc::IObject objArchive;
    Alembic::AbcCoreFactory::IFactory::CoreType coreType;
    PathMapPtr pathMap;
    // All the assignments of the cache
    AssignmentsPtr assignments;

	AttributesPtr attributes;
    // List of materials with both surface and displacement shaders.
    ShaderSetPtr shaderSet;

};

class AlembicCache :
    public FnGeolibUtil::AttributeKeyedCache< ArchiveAndFriends >
{
public:
    AlembicCache() :
        FnGeolibUtil::AttributeKeyedCache< ArchiveAndFriends >(100, 1000)
    {
        factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
    }

    // number of streams to open per file
    void setNumStreams( std::size_t iNumStreams )
    {
        factory.setOgawaNumStreams( iNumStreams );
    }

private:
    AlembicCache::IMPLPtr createValue(const FnAttribute::Attribute & iAttr)
    {
        AlembicCache::IMPLPtr val;
        Alembic::AbcCoreFactory::IFactory::CoreType coreType;

        Alembic::Abc::IArchive archive =
            getArchive(
                    factory,
                    FnAttribute::StringAttribute(iAttr).getValue(),
                    coreType);

        if ( archive.valid() )
        {
            val.reset(new ArchiveAndFriends(archive, coreType));
        }

        return val;
    }

    Alembic::AbcCoreFactory::IFactory factory;
};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

AlembicCache g_cache;

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

class WalterInPrivateData : public Foundry::Katana::GeolibPrivateData
{
public:
    WalterInPrivateData() :
        Foundry::Katana::GeolibPrivateData(),
        material(NULL)
    {
    }

    virtual ~WalterInPrivateData(){};

    WalterIn::AbcCookPtr cookPtr;
    PathMapPtr pathMap;
    // All the assignments of the cache
    AssignmentsPtr assignments;
    // All the attributes
    AttributesPtr attributes;
    // List of materials with both surface and displacement shaders.
    ShaderSetPtr shaderSet;
    // TODO: use reference
    std::string root;
    // Pointer to material inside shaderSet. Used to generate virtual materials.
    const Material* material;

    // We save the parent's material to be able to skip material assignment if
    // it's already assigned to the parent and produce clean data.
    std::string parentMaterial;
};

WalterIn::AbcCookPtr getTopObject(
        const FnAttribute::StringAttribute & iAttr,
        const FnAttribute::StringAttribute & iRootAttr,
        std::string & oOpType,
        PathMapPtr & oPathMap,
        AssignmentsPtr& oAssignments,
        AttributesPtr& oAttributes,
        ShaderSetPtr& oShaderSet)
{
    WalterIn::AbcCookPtr retVal;
    Alembic::Abc::IObject obj;
    AlembicCache::IMPLPtr entry = g_cache.getValue(iAttr);

    if (!entry || !entry->objArchive.valid())
    {
        return retVal;
    }

    obj = entry->objArchive;

    std::string pathToRoot;
    if (iRootAttr.isValid())
    {
        pathToRoot = iRootAttr.getValue();
    }

    if (!pathToRoot.empty())
    {
        std::vector< std::string > tokens;
        pathToRoot = pystring::strip(pathToRoot, " /\n\t");
        pystring::split(pathToRoot, tokens, "/");
        std::vector< std::string >::const_iterator i;
        for (i = tokens.begin(); i != tokens.end(); ++i)
        {
            if (!obj.getChildHeader(*i))
            {
                return retVal;
            }
            obj = obj.getChild(*i);
        }
    }

    // TODO, do something with entry->coreType
    if (entry->coreType == Alembic::AbcCoreFactory::IFactory::kOgawa)
    {
        oOpType = "WalterInOgawa";
    }
    else if (entry->coreType == Alembic::AbcCoreFactory::IFactory::kHDF5)
    {
        oOpType = "WalterInHDF";
    }
    else if (entry->coreType == Alembic::AbcCoreFactory::IFactory::kLayer)
    {
        oOpType = "WalterInLayer";
    }

    retVal = WalterIn::AbcCookPtr(new WalterIn::AbcCook());
    retVal->objPtr = Alembic::Abc::IObjectPtr(new Alembic::Abc::IObject(obj));

    if (retVal->objPtr->valid())
    {
        Alembic::Abc::ICompoundProperty prop = retVal->objPtr->getProperties();
        const Alembic::Abc::PropertyHeader * childBnds =
            prop.getPropertyHeader(".childBnds");
        if (childBnds)
        {
            FnAttribute::GroupBuilder staticGb;
            scalarPropertyToAttr(prop, *childBnds, "bound", retVal, staticGb);
            retVal->staticGroup = staticGb.build();
        }
    }

    oPathMap = entry->pathMap;
    oAssignments = entry->assignments;
    oAttributes = entry->attributes;
    oShaderSet = entry->shaderSet;
    return retVal;
}


// Report fatal errors to the scene graph and halt traversal.
static void reportAlembicError(Foundry::Katana::GeolibCookInterface &interface,
                               const std::string &filePath,
                               const std::string &errorMessage)
{
    interface.setAttr("type", FnAttribute::StringAttribute("error"));
    interface.setAttr("errorMessage",
                      FnAttribute::StringAttribute("Can not load: " +
                                                   filePath + " : " +
                                                   errorMessage));
    interface.stopChildTraversal();
    return;
}

// Extracts bookkeeping information from an AbcCook pointer and sets them
// under the 'info' attribute at the root location.
static void fillRootInfo(Foundry::Katana::GeolibCookInterface &interface,
                         WalterIn::AbcCookPtr ioCookPtr)
{
    if (!ioCookPtr->objPtr || ioCookPtr->objPtr->getParent().valid())
    {
        return;
    }

    double fps = FnAttribute::DoubleAttribute(interface.getOpArg(
        "fps")).getValue(24.0, false);

    std::vector<double> frameRange(2);
    frameRange[0] = DBL_MAX;
    frameRange[1] = -DBL_MAX;

    Alembic::Abc::IArchive archive = ioCookPtr->objPtr->getArchive();
    uint32_t numSampling = archive.getNumTimeSamplings();

    // ignore the default time sampling at index 0
    for (uint32_t i = 1; i < numSampling; ++i)
    {
        Alembic::AbcCoreAbstract::index_t maxIndex =
            archive.getMaxNumSamplesForTimeSamplingIndex(i);

        if (maxIndex < 2 || maxIndex == INDEX_UNKNOWN)
        {
            continue;
        }

        Alembic::AbcCoreAbstract::TimeSamplingPtr ts =
            archive.getTimeSampling(i);

        std::vector<double> samples(maxIndex);
        for (Alembic::AbcCoreAbstract::index_t j = 0; j < maxIndex; ++j)
        {
            samples[j] = ts->getSampleTime(j) * fps;
        }

        if (samples.front() < frameRange[0])
        {
            frameRange[0] = samples.front();
        }

        if (samples.back() < frameRange[0])
        {
            frameRange[0] = samples.back();
        }

        if (samples.front() > frameRange[1])
        {
            frameRange[1] = samples.front();
        }

        if (samples.back() > frameRange[1])
        {
            frameRange[1] = samples.back();
        }

        std::stringstream strm;
        strm << "info.frameSamples" << i;
        interface.setAttr(strm.str().c_str(), FnAttribute::DoubleAttribute(
            &(samples.front()), samples.size(), 1));
    }

    if (frameRange[0] < frameRange[1])
    {
        interface.setAttr("info.frameRange", FnAttribute::DoubleAttribute(
            &(frameRange.front()), frameRange.size(), 1));
    }
}

// Builds and returns a GroupAttribute from an AbcCookPtr with all its
// static and non-static properties.
static FnAttribute::GroupAttribute getAllProps(
    const Foundry::Katana::GeolibCookInterface &interface,
    const WalterIn::AbcCookPtr &ioCookPtr)
{
    if (!ioCookPtr->arrayProps.empty() || !ioCookPtr->scalarProps.empty() ||
        ioCookPtr->objPtr || ioCookPtr->visProp.valid())
    {
        // let's get our other args which we will need for our animated
        // reads
        WalterIn::OpArgs args;
        args.currentTime = FnAttribute::FloatAttribute(interface.getOpArg(
            "system.timeSlice.currentTime")).getValue(0.0, false);
        args.shutterOpen = FnAttribute::FloatAttribute(interface.getOpArg(
            "system.timeSlice.shutterOpen")).getValue(0.0, false);
        args.shutterClose = FnAttribute::FloatAttribute(interface.getOpArg(
            "system.timeSlice.shutterClose")).getValue(0.0, false);
        args.numSamples = FnAttribute::IntAttribute(interface.getOpArg(
            "system.timeSlice.numSamples")).getValue(0, false);

        args.fps = FnAttribute::DoubleAttribute(interface.getOpArg(
            "fps")).getValue(24.0, false);

        std::string beyondRange =
            FnAttribute::StringAttribute(interface.getOpArg(
                "beyondRangeBehavior")).getValue("", false);

        if (beyondRange == "error")
        {
            args.behavior = WalterIn::OpArgs::kError;
        }
        else if (beyondRange == "hold")
        {
            args.behavior = WalterIn::OpArgs::kHold;
        }

        const int useOnlyShutterOpenCloseTimesFlag =
            FnAttribute::IntAttribute(interface.getOpArg(
                "useOnlyShutterOpenCloseTimes")).getValue(0, false);
        args.useOnlyShutterOpenCloseTimes =
            (useOnlyShutterOpenCloseTimesFlag == 1);

        FnAttribute::GroupBuilder bld;

        {
            // Query to ioCookPtr->staticGroup should be locked. It's not locked
            // in the original Alembic because they don't have setAttributes()
            // and they don't access it from there.
            boost::lock_guard<boost::mutex> lock(ioCookPtr->mutex);
            bld.deepUpdate(ioCookPtr->staticGroup);
        }

        Alembic::Abc::IUInt64ArrayProperty idsProp;
        Alembic::Abc::IUInt64ArrayProperty * iIdsProperty = NULL;
        if (ioCookPtr->objPtr)
        {
            const Alembic::AbcCoreAbstract::ObjectHeader & header =
                ioCookPtr->objPtr->getHeader();
            if (Alembic::AbcGeom::IPoints::matches(header))
            {
                Alembic::AbcGeom::IPointsPtr objPtr(
                    new Alembic::AbcGeom::IPoints(*(ioCookPtr->objPtr),
                                                  Alembic::AbcGeom::kWrapExisting));

                Alembic::AbcGeom::IPointsSchema schema = objPtr->getSchema();
                idsProp = schema.getIdsProperty();
                if (idsProp.valid())
                {
                    iIdsProperty = &idsProp;
                }
            }
        }

        for (std::vector< WalterIn::ArrayProp >::iterator at =
             ioCookPtr->arrayProps.begin();
             at != ioCookPtr->arrayProps.end(); ++at)
        {
            if (iIdsProperty && at->prop.getPtr() != iIdsProperty->getPtr())
            {
                WalterIn::arrayPropertyToAttr(*at, args, bld, iIdsProperty);
            }
            else
            {
                WalterIn::arrayPropertyToAttr(*at, args, bld);            
            }
        }

        for (std::vector< WalterIn::ScalarProp >::iterator st =
             ioCookPtr->scalarProps.begin();
             st != ioCookPtr->scalarProps.end(); ++st)
        {
            WalterIn::scalarPropertyToAttr(*st, args, bld);
        }

        for (std::vector< WalterIn::IndexedGeomParamPair >::iterator ft =
             ioCookPtr->forcedExpandProps.begin();
             ft != ioCookPtr->forcedExpandProps.end(); ++ft)
        {
            WalterIn::indexedParamToAttr(*ft, args, bld);
        }

        if (ioCookPtr->animatedSchema)
        {
            WalterIn::evalObject(ioCookPtr, args, bld, iIdsProperty);
        }

        return bld.build();

    }
    // no animation set our static attrs
    return ioCookPtr->staticGroup;
}

static void setAllAttrs(Foundry::Katana::GeolibCookInterface &interface,
                        const FnAttribute::GroupAttribute &attrs)
{
    for (int64_t i = 0; i < attrs.getNumberOfChildren(); ++i)
    {
        interface.setAttr(attrs.getChildName(i), attrs.getChildByIndex(i));
    }
}

// Set the material. Returns material assigned on the current object or parent.
static std::string setAssignment(
        Foundry::Katana::GeolibCookInterface &interface,
        const std::string& iName,
        const AssignmentsPtr& iAssignments,
        const std::string& iRoot,
        const std::string& iParentMaterial)
{
    const std::string shader =
        getShaderAssignment(iName, iAssignments, "shader");
    const std::string displacement =
        getShaderAssignment(iName, iAssignments, "displacement");

    std::vector<std::string> names;
    if (!shader.empty())
    {
        names.push_back(shader);
    }
    if (!displacement.empty())
    {
        names.push_back(displacement);
    }

    if (names.empty())
    {
        return "";
    }

    const std::string material =
        iRoot + "/materials/" + boost::algorithm::join(names, "_");

    if (material != iParentMaterial)
    {
        // Assign material only if the parent doesn't have the same material.
        // TODO: use ioCookPtr->staticGroup for that
        interface.setAttr(
            "materialAssign",
            FnAttribute::StringAttribute(material));

    }

    return material;
}

// Set the material. Returns material assigned on the current object or parent.
static std::string setAttributes(
        const WalterIn::AbcCookPtr &ioCookPtr,
        const AssignmentsPtr& iAssignments,
        const AttributesPtr& iAttributes,
        const std::string& iRoot,
        const std::string& iParentAttributes)
{
    const std::string& name = ioCookPtr->objPtr->getFullName();

    const std::string attributes =
        getShaderAssignment(name, iAssignments, "attribute");

    if (attributes.empty())
    {
        return "";
    }

    auto it = iAttributes->find(attributes);
    if (it != iAttributes->end() && attributes != iParentAttributes)
    {
        FnAttribute::GroupBuilder walterBuilder;

        walterBuilder.set("arnoldStatements", it->second.first);

        // Set the User Data. KTOA needs it at "geometry.arbitrary".
        walterBuilder.set("geometry.arbitrary", it->second.second);

        // Query to ioCookPtr->staticGroup should be locked.
        boost::lock_guard<boost::mutex> lock(ioCookPtr->mutex);

        // Katana magic to merge groups. We don't use `set`, `update`,
        // `interface::setAttr` because it removes the children and replaces
        // entire group and there is a chance that we loose something important
        // that was there from the original Alembic because we fill the
        // "geometry.arbitrary" group from two places. From Arnold code and the
        // code related to Walter Override. We loosed attributes when there were
        // intersections because Katana overrode whole group. It's fixed with
        // following Katana magic.
        FnAttribute::GroupBuilder groupBuilder;
        groupBuilder.update(ioCookPtr->staticGroup);
        groupBuilder.deepUpdate(walterBuilder.build());

        ioCookPtr->staticGroup = groupBuilder.build();
    }

    return attributes;
}

// Set the material. Returns materials assigned on the current object or parent.
// This flavor of the setAttributes function is to be used on already created 
// Katana locations (usually generated by a WalterInOp).
static std::string setAttributes(
        Foundry::Katana::GeolibCookInterface &iInterface,
        const std::string& iName,
        const AssignmentsPtr& iAssignments,
        const AttributesPtr& iAttributes,
        const std::string& iRoot,
        const std::string& iParentAttributes)
{
    const std::string attributes =
        getShaderAssignment(iName, iAssignments, "attribute");

    if (attributes.empty())
    {
        return "";
    }

    auto it = iAttributes->find(attributes);
    if (it != iAttributes->end() && attributes != iParentAttributes)
    {
        iInterface.setAttr("arnoldStatements", it->second.first);
        iInterface.setAttr("geometry.arbitrary", it->second.second);
    }

    return attributes;
}

// Utility function to handle extracting all the properties from an AbcCookPtr
// as a GroupAttribute and setting them on the current location.
// Returns material name assigned on the current object or parent.
static void fillAllProps(
        Foundry::Katana::GeolibCookInterface &interface,
        WalterIn::AbcCookPtr ioCookPtr,
        const AssignmentsPtr& iAssignments,
        const AttributesPtr& iAttributes,
        const std::string& iRoot,
        const std::string& iParentMaterial,
        std::string& oAssignedMaterial,
        std::string& oAssignedAttributes)
{
    fillRootInfo(interface, ioCookPtr);

    oAssignedMaterial = setAssignment(
        interface,
        ioCookPtr->objPtr->getFullName(),
        iAssignments,
        iRoot,
        iParentMaterial);

    oAssignedAttributes = setAttributes(
        ioCookPtr, iAssignments, iAttributes, iRoot, iParentMaterial);

    FnAttribute::GroupAttribute allProps = getAllProps(interface, ioCookPtr);
    setAllAttrs(interface, allProps);
}

static void cookAlembic(
        Foundry::Katana::GeolibCookInterface &interface,
        const std::string & iOpType,
        WalterIn::AbcCookPtr ioCookPtr,
        PathMapPtr & iPathMap,
        const AssignmentsPtr& iAssignments,
        const AttributesPtr& iAttributes,
        const ShaderSetPtr& iShaderSet,
        const std::string& iRoot,
        const std::string& iParentMaterial)
{
    if (!ioCookPtr->staticGroup.isValid())
    {
        boost::lock_guard<boost::mutex> lock(ioCookPtr->mutex);

        if (!ioCookPtr->staticGroup.isValid())
        {
            FnAttribute::GroupBuilder staticBld;
            WalterIn::initAbcCook(ioCookPtr, staticBld);
            ioCookPtr->staticGroup = staticBld.build();
        }
    }

    std::string assignedMaterial;
    std::string assignedAttributes;
    fillAllProps(
            interface,
            ioCookPtr,
            iAssignments,
            iAttributes,
            iRoot,
            iParentMaterial,
            assignedMaterial,
            assignedAttributes);

    // Check if we need to add "forceExpand".
    FnAttribute::IntAttribute addForceExpandAttr =
            interface.getOpArg("addForceExpand");

    // In case we found "addForceExpand" we have to remove it from the opArgs
    // so that it will be set only by the first-level children
    FnAttribute::Attribute childOpArgs = interface.getOpArg();

    if (addForceExpandAttr.isValid() && addForceExpandAttr.getValue(1, false))
    {
        interface.setAttr("forceExpand", FnAttribute::IntAttribute(1));

        FnAttribute::GroupBuilder newOpArgs;
        newOpArgs.update(childOpArgs);
        newOpArgs.del("addForceExpand");
        childOpArgs = newOpArgs.build();
    }

    // Check wether we need to copy the bounds from our parent
    FnAttribute::DoubleAttribute parentBound =
        interface.getOpArg("parentBound");
    if (parentBound.isValid())
    {
        FnAttribute::GroupBuilder boundsFromParentOpArgsGb;
        boundsFromParentOpArgsGb.set("parentBound", parentBound);
        interface.execOp("BoundsFromParent", boundsFromParentOpArgsGb.build());

        // Remove the parent bound from the OpArgs since we do not want the
        // grandchildren to set it.
        FnAttribute::GroupBuilder newOpArgs;
        newOpArgs.update(childOpArgs);
        newOpArgs.del("parentBound");
        childOpArgs = newOpArgs.build();
    }

    std::size_t numChildren = ioCookPtr->objPtr->getNumChildren();

    for (std::size_t i = 0; i < numChildren; ++i)
    {
        const Alembic::AbcCoreAbstract::ObjectHeader & header =
            ioCookPtr->objPtr->getChildHeader(i);

        WalterInPrivateData * childData = new WalterInPrivateData();

        childData->pathMap = iPathMap;
        childData->assignments = iAssignments;
        childData->attributes = iAttributes;
        childData->shaderSet = iShaderSet;
        childData->cookPtr = iPathMap->get(header.getFullName(),
            *ioCookPtr->objPtr, header.getName());
        childData->root = iRoot;
        childData->parentMaterial = assignedMaterial;

        interface.createChild(header.getName(), iOpType, childOpArgs,
            Foundry::Katana::GeolibCookInterface::ResetRootAuto, childData,
            WalterInPrivateData::Delete);
    }

    if (ioCookPtr->objPtr->getFullName() == "/materials")
    {
        // Add virtual shaders.
        BOOST_FOREACH (const auto& material, *iShaderSet)
        {
            std::string name = material.first + "_" + material.second;

            WalterInPrivateData * childData = new WalterInPrivateData();

            childData->pathMap = iPathMap;
            childData->root = iRoot;
            childData->material = &material;
            childData->cookPtr = ioCookPtr;

            interface.createChild(
                    name,
                    "WalterInVirtualMat",
                    childOpArgs,
                    Foundry::Katana::GeolibCookInterface::ResetRootAuto,
                    childData,
                    WalterInPrivateData::Delete);
        }
    }
}

class WalterInHDFOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        interface.setThreading(Foundry::Katana::GeolibSetupInterface::ThreadModeGlobalUnsafe);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {
        WalterInPrivateData *privateData =
            static_cast<WalterInPrivateData *>(interface.getPrivateData());

        if (!privateData || !privateData->cookPtr || !privateData->pathMap)
        {
            interface.stopChildTraversal();
            return;
        }

        cookAlembic(
                interface,
                "WalterInHDF",
                privateData->cookPtr,
                privateData->pathMap,
                privateData->assignments,
                privateData->attributes,
                privateData->shaderSet,
                privateData->root,
                privateData->parentMaterial);
    }
};

class WalterAssignOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface& interface)
    {
        interface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface& interface)
    {
        // In order to run the Op we need a valid CEL statement
        FnAttribute::StringAttribute celAttr = interface.getOpArg("CEL");
        if (!celAttr.isValid())
        {
            interface.stopChildTraversal();
            return;
        }

        FnGeolibServices::FnGeolibCookInterfaceUtils::MatchesCELInfo info;
        FnGeolibServices::FnGeolibCookInterfaceUtils::matchesCEL(
            info, interface, celAttr);
        if (!info.canMatchChildren)
        {
            interface.stopChildTraversal();
        }

        // If the CEL doesn't match the current location, stop cooking
        if (!info.matches)
        {
            return;
        }

        FnAttribute::StringAttribute fileAttr = interface.getOpArg("fileName");
        std::vector<std::string> archives;
        if (fileAttr.isValid())
        {
            splitArchives(fileAttr.getValue(), archives);
        }

        if (archives.empty())
        {
            interface.setAttr("type", FnAttribute::StringAttribute("error"));
            interface.setAttr("errorMessage",
                FnAttribute::StringAttribute("No file specified"));
            interface.stopChildTraversal();
            return;
        }

        FnAttribute::StringAttribute rootAttr = interface.getOpArg("root");
        if(!rootAttr.isValid())
        {
            return;
        }

        const std::string root = rootAttr.getValue();
        const std::string inputLocation = interface.getInputLocationPath();

        if(inputLocation.size() < root.size() || inputLocation.size() == root.size())
        {
            return;
        }

        const std::string objPath = inputLocation.substr(root.size());
        AlembicCache::IMPLPtr entry = g_cache.getValue(fileAttr);

        setAssignment(interface, objPath, entry->assignments, root, "");
        setAttributes(
            interface,
            objPath,
            entry->assignments,
            entry->attributes,
            root,
            "");
    }
};

class WalterInOgawaOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        interface.setThreading(Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {
        WalterInPrivateData *privateData =
            static_cast<WalterInPrivateData *>(interface.getPrivateData());

        if (!privateData || !privateData->cookPtr || !privateData->pathMap)
        {
            interface.stopChildTraversal();
            return;
        }

        cookAlembic(
                interface,
                "WalterInOgawa",
                privateData->cookPtr,
                privateData->pathMap,
                privateData->assignments,
                privateData->attributes,
                privateData->shaderSet,
                privateData->root,
                privateData->parentMaterial);
    }
};

// Virtual shaders.
class WalterInVirtualMatOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        interface.setThreading(
                Foundry::Katana::GeolibSetupInterface::ThreadModeConcurrent);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {
        WalterInPrivateData *privateData =
            static_cast<WalterInPrivateData *>(interface.getPrivateData());

        if (!privateData ||
            !privateData->cookPtr ||
            !privateData->pathMap ||
            !privateData->material)
        {
            interface.stopChildTraversal();
            return;
        }

        const std::string& shaderName = privateData->material->first;
        const std::string& displacementName = privateData->material->second;
        const std::string& parentName =
            privateData->cookPtr->objPtr->getFullName();

        std::string fullShaderName = parentName + "/" + shaderName;
        std::string fullDisplacementName = displacementName + "/" + shaderName;

        WalterIn::AbcCookPtr shaderCookPtr = privateData->pathMap->get(
                fullShaderName, *privateData->cookPtr->objPtr, shaderName);
        WalterIn::AbcCookPtr displacementCookPtr = privateData->pathMap->get(
                fullDisplacementName,
                *privateData->cookPtr->objPtr,
                displacementName);

        if (!shaderCookPtr || !displacementCookPtr)
        {
            return;
        }

        FnAttribute::GroupBuilder staticBld;
        WalterIn::initAbcCook(shaderCookPtr, staticBld);
        WalterIn::initAbcCook(displacementCookPtr, staticBld);
        setAllAttrs(interface, staticBld.build());
    }
};

class WalterInLayerOp : public WalterInOgawaOp
{
};

class WalterInOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface &interface)
    {
        // FIXME: Should this top level Alembic op being more optimistic?
        // (I.e., concurrent).  For the pure ogawa situation, it would be
        // nice if ThreadModeGlobalUnsafe was *never* encountered by the runtime.
        interface.setThreading(Foundry::Katana::GeolibSetupInterface::ThreadModeGlobalUnsafe);
    }

    static void cook(Foundry::Katana::GeolibCookInterface &interface)
    {
        FnAttribute::StringAttribute fileAttr = interface.getOpArg("fileName");
        std::vector<std::string> archives;
        if (fileAttr.isValid())
        {
            splitArchives(fileAttr.getValue(), archives);
        }

        if (archives.empty())
        {
            interface.setAttr("type", FnAttribute::StringAttribute("error"));
            interface.setAttr("errorMessage",
                FnAttribute::StringAttribute("No file specified"));
            interface.stopChildTraversal();
            return;
        }
        else
        {
            // Check if layers have a usd file. In this way it should be a USD
            // variant of WalterIn.
            bool isUSD = false;

            // Check if we have to force USD engine to load this node.
            auto forceUSDAttr = interface.getOpArg("forceUSD");
            int forceUSD =
                FnAttribute::IntAttribute(forceUSDAttr).getValue(0, false);

            if (!forceUSD)
            {
                for (const std::string& f : archives)
                {
                    if (!boost::algorithm::iends_with(f, ".abc"))
                    {
                        isUSD = true;
                        break;
                    }
                }
            }

            if (isUSD || forceUSD)
            {
                cookUSDRoot(interface, archives);
                return;
            }
        }

        FnAttribute::StringAttribute pathFromRootAttr =
            interface.getOpArg("pathFromRoot");

        PathMapPtr pathMap;
        AssignmentsPtr assignments;
        AttributesPtr attributes;
        ShaderSetPtr shaderSet;
        std::string opType;

        WalterIn::AbcCookPtr curObj =
            getTopObject(
                    fileAttr,
                    pathFromRootAttr,
                    opType,
                    pathMap,
                    assignments,
                    attributes,
                    shaderSet);

        const std::string filePath = fileAttr.getValue();
        // Check and report our error conditions
        if(!curObj)
        {
            reportAlembicError(interface, filePath, "Top-level object was NULL");
            return;
        }

        if(!curObj->objPtr)
        {
            reportAlembicError(interface, filePath, "Got top-level object but objPtr was NULL");
            return;
        }

        if(!curObj->objPtr->valid())
        {
            reportAlembicError(interface, filePath, "Top-level objPtr was invalid");
            return;
        }

        if(opType == "")
        {
            reportAlembicError(interface, filePath, "Specified opType was empty");
            return;
        }

        if(!pathMap)
        {
            reportAlembicError(interface, filePath, "Path map was NULL");
            return;
        }

        // Check if we need to set bounds. On the root we can set it here,
        // for the children we pass it as opArg for them to deal with it.
        std::string addBounds = FnAttribute::StringAttribute(
            interface.getOpArg("addBounds")).getValue("none", false);
        FnAttribute::GroupAttribute childOpArgs = interface.getOpArg();

        if (addBounds != "none")
        {
            fillRootInfo(interface, curObj);
            FnAttribute::GroupAttribute allProps = getAllProps(interface,
                                                               curObj);

            FnAttribute::DoubleAttribute rootBound =
                allProps.getChildByName("bound");

            if (rootBound.isValid()
                && (addBounds == "both" || addBounds == "children"))
            {
                // If we want the children to inherit the bound we need to
                // pass it via the OpArgs
                FnAttribute::GroupBuilder gb;
                gb.update(childOpArgs);
                gb.set("parentBound", rootBound);
                childOpArgs = gb.build();
            }

            if (addBounds == "children")
            {
                // remove bound from the attribute to set
                FnAttribute::GroupBuilder gb;
                gb.update(allProps);
                gb.del("bound");
                allProps = gb.build();
            }
            setAllAttrs(interface, allProps);
        }

        // Check if we need to set Arnold defaults. We should do it on the root
        // only.
        std::string addDefaultAttrs = FnAttribute::StringAttribute(
            interface.getOpArg("addDefaultAttrs")).getValue("Arnold", false);
        if (addDefaultAttrs == "Arnold")
        {
            setArnoldDefaults(interface);
        }

        // invoke the children
        std::size_t numChildren = curObj->objPtr->getNumChildren();

        for (std::size_t i = 0; i < numChildren; ++i)
        {
            const Alembic::AbcCoreAbstract::ObjectHeader & header =
                curObj->objPtr->getChildHeader(i);

            WalterInPrivateData * childData = new WalterInPrivateData();

            childData->pathMap = pathMap;
            childData->assignments = assignments;
            childData->attributes = attributes;
            childData->shaderSet = shaderSet;
            childData->cookPtr = pathMap->get(header.getFullName(),
                *curObj->objPtr, header.getName());
            childData->root = interface.getOutputLocationPath();

            interface.createChild(header.getName(), opType,
                childOpArgs,
                Foundry::Katana::GeolibCookInterface::ResetRootAuto, childData,
                WalterInPrivateData::Delete);
        }
    }

    static void flush()
    {
        g_cache.clear();
    }
};

DEFINE_GEOLIBOP_PLUGIN(WalterInOp)
DEFINE_GEOLIBOP_PLUGIN(WalterAssignOp)
DEFINE_GEOLIBOP_PLUGIN(WalterInHDFOp)
DEFINE_GEOLIBOP_PLUGIN(WalterInOgawaOp)
DEFINE_GEOLIBOP_PLUGIN(WalterInLayerOp)
DEFINE_GEOLIBOP_PLUGIN(WalterInVirtualMatOp)


class CameraAndLightPathCache :
    public FnGeolibUtil::AttributeKeyedCache< FnAttribute::GroupAttribute >
{
private:
    CameraAndLightPathCache::IMPLPtr createValue(const FnAttribute::Attribute & iAttr)
    {
        CameraAndLightPathCache::IMPLPtr val(new FnAttribute::GroupAttribute);
        Alembic::AbcCoreFactory::IFactory factory;


        Alembic::AbcCoreFactory::IFactory::CoreType coreType;

        Alembic::Abc::IArchive archive =
            getArchive(
                    factory,
                    FnAttribute::StringAttribute(iAttr).getValue("", false),
                    coreType);

        if (archive.valid())
        {
            std::vector<std::string> cameras, lights;
            walk(archive.getTop(), cameras, lights);

            val.reset(new FnAttribute::GroupAttribute(
                    "cameras", FnAttribute::StringAttribute(cameras, 1),
                    "lights", FnAttribute::StringAttribute(lights, 1),
                            true));
        }

        return val;
    }

    void walk(Alembic::Abc::IObject object, std::vector<std::string> & cameras,
            std::vector<std::string> & lights)
    {
        if (!object.valid())
        {
            return;
        }

        if (Alembic::AbcGeom::ICamera::matches(object.getHeader()))
        {
            cameras.push_back(object.getFullName());
        }
        else if (Alembic::AbcGeom::ILight::matches(object.getHeader()))
        {
            lights.push_back(object.getFullName());
        }

        for (size_t i = 0, e = object.getNumChildren(); i < e; ++i)
        {
            walk(object.getChild(i), cameras, lights);
        }
    }
};

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#endif

CameraAndLightPathCache g_cameraAndLightCache;

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

class WalterInAddToLightAndCameraListsOp : public Foundry::Katana::GeolibOp
{
public:
    static void setup(Foundry::Katana::GeolibSetupInterface& interface)
    {
        interface.setThreading(
            Foundry::Katana::GeolibSetupInterface::ThreadModeGlobalUnsafe);
    }

    static void cook(Foundry::Katana::GeolibCookInterface& interface)
    {
        interface.stopChildTraversal();

        bool setLightList = false;
        bool setCamList = false;

        FnAttribute::IntAttribute setLightListAttr =
            FnAttribute::IntAttribute(interface.getOpArg("setLightList"));
        if(setLightListAttr.isValid())
        {
            setLightList = static_cast<bool>(setLightListAttr.getValue());
        }

        FnAttribute::IntAttribute setCamListAttr =
            FnAttribute::IntAttribute(interface.getOpArg("setCameraList"));
        if(setCamListAttr.isValid())
        {
            setCamList = static_cast<bool>(setCamListAttr.getValue());
        }

        FnAttribute::GroupAttribute pathsGroup =
            *(g_cameraAndLightCache.getValue(interface.getOpArg("fileName")));

        if (!pathsGroup.isValid())
        {
            return;
        }

        std::string scenegraphPath =
            FnAttribute::StringAttribute(interface.getOpArg("scenegraphPath"))
                .getValue("/root", false);

        if (setLightList)
        {
            // Add lights
            FnAttribute::StringAttribute ligthsAttr =
                pathsGroup.getChildByName("lights");

            if (ligthsAttr.isValid() && ligthsAttr.getNumberOfValues())
            {
                std::vector<std::string> lightList;
                lightList.reserve(ligthsAttr.getNumberOfValues());

                FnAttribute::StringAttribute::array_type values =
                    ligthsAttr.getNearestSample(0.0);

                for (FnAttribute::StringAttribute::array_type::const_iterator
                         I = values.begin(),
                         E = values.end();
                     I != E;
                     ++I)
                {
                    const std::string lightPath = scenegraphPath + (*I);

                    lightList.emplace_back(scenegraphPath + (*I));

                    const std::string prefix =
                        pystring::replace(lightPath, "/", "_");

                    interface.extendAttr(
                        "lightList." + prefix + ".path",
                        FnAttribute::StringAttribute(lightPath),
                        "",
                        false);
                }
            }
        }

        if (setCamList)
        {
            // Add  cameras
            FnAttribute::StringAttribute camerasAttr =
                pathsGroup.getChildByName("cameras");

            if (camerasAttr.isValid() && camerasAttr.getNumberOfValues())
            {
                std::vector<std::string> cameraList;
                cameraList.reserve(camerasAttr.getNumberOfValues());

                FnAttribute::StringAttribute::array_type values =
                    camerasAttr.getNearestSample(0.0);

                for (FnAttribute::StringAttribute::array_type::const_iterator
                         I = values.begin(),
                         E = values.end();
                     I != E;
                     ++I)
                {
                    cameraList.emplace_back(scenegraphPath + (*I));
                }

                interface.extendAttr(
                    "globals.cameraList",
                    FnAttribute::StringAttribute(cameraList),
                    "",
                    false);
            }
        }
    }
};

DEFINE_GEOLIBOP_PLUGIN(WalterInAddToLightAndCameraListsOp)






} // anonymous

void registerPlugins()
{
    H5dont_atexit();
    REGISTER_PLUGIN(WalterInOp, "WalterIn", 0, 1);
    REGISTER_PLUGIN(WalterAssignOp, "WalterAssign", 0, 1);
    REGISTER_PLUGIN(WalterInHDFOp, "WalterInHDF", 0, 1);
    REGISTER_PLUGIN(WalterInOgawaOp, "WalterInOgawa", 0, 1);
    REGISTER_PLUGIN(WalterInLayerOp, "WalterInLayer", 0, 1);
    REGISTER_PLUGIN(WalterInVirtualMatOp, "WalterInVirtualMat", 0, 1);
    REGISTER_PLUGIN(WalterInAddToLightAndCameraListsOp,
                "WalterInAddToLightAndCameraLists", 0, 1);

    registerUSDPlugins();
}
