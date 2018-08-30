
#include "translators/shape/ShapeTranslator.h"
#include "mtoaVersion.h"

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
    // Export Arnold shader and displacement from the Maya shaders and displacements nodes
    // connected to the shape.
    bool ExportMayaShadingGraph();

    // Export node graph connect to this plug
    void ExportConnections(
            const char* abcnode,
            const MPlug& shaderPlug);

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
