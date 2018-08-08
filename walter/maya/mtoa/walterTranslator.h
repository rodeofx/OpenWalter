
#include "translators/shape/ShapeTranslator.h"
#include "mtoaVersion.h"

#include <json/value.h>
#include <translators/NodeTranslator.h>

class CWalterStandinTranslator
   :   public CShapeTranslator
{
public:
#if WALTER_MTOA_VERSION >= 10400
	CWalterStandinTranslator();
	void ExportMotion(AtNode*);
#else
	virtual void Update(AtNode* procedural);
	void ExportMotion(AtNode*, unsigned int);
#endif
	AtNode* CreateArnoldNodes();
	virtual void Export(AtNode* procedural);
	static void NodeInitializer(CAbTranslator context);

	static void* creator()
	{
		return new CWalterStandinTranslator();
	}
protected:

    void ProcessRenderFlagsCustom(AtNode* node);
   void ExportBoundingBox(AtNode* procedural);
   void ExportStandinsShaders(AtNode* procedural);
   void ExportProcedural(AtNode* procedural, bool update);

   virtual void ExportShaders();

protected:
   MFnDagNode m_DagNode;

private:
    // Generate the assignation JSON string from the layersAssignation attribute
    bool GenerateLayersAssignation(
            MString& shaders,
            MString& displacements,
            MString& layers,
            MString& attributes);

    // Add the connection to the JSON structure.
    void AppendConnection(
            const char* abcnode,
            const MPlug& shaderPlug,
            Json::Value& root);

    // Adds an attribute to the attribute json structure.
    void AppendAttributes(
            const char* abcnode,
            const MPlug& attrPlug,
            Json::Value& root)const;

    // Computes visibility attributes based on the node's attributes. Returns
    // negative when the visibility can't be reached.
    int ComputeWalterVisibility(const MFnDependencyNode& node)const;

#if WALTER_MTOA_VERSION >= 10400
    // The node which is at the root of this translator.
    AtNode* GetArnoldRootNode();
#endif

#if WALTER_MTOA_VERSION >= 10400
    void ExportFrame(AtNode* node);
#else
    void ExportFrame(AtNode* node, unsigned int step);
#endif

    // Shortcut to the node which is at the root of this translator. Do not
    // delete, it is supposed to be one of the translators nodes.
    AtNode *m_arnoldRootNode;
};
