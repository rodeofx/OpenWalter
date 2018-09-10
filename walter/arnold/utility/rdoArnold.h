#ifndef __RDO_ARNOLD_H__
#define __RDO_ARNOLD_H__

#include <ai.h>

#if AI_VERSION_ARCH_NUM == 5

#define RDO_WALTER_PROC "walter"

// Point/Vector
#define RDO_AI_TYPE_VECTOR AI_TYPE_VECTOR
#define RDO_AI_TYPE_VECTOR2 AI_TYPE_VECTOR2
#define RDO_STR_TYPE_VECTOR2 "VECTOR2"
#define RDO_STR_TYPE_VECTOR "VECTOR"

typedef AtVector RdoAtVector;
typedef AtVector2 RdoAtVector2;

// Node
void RdoAiNodeSetVec(AtNode* node, const char* name, float x, float y, float z);
void RdoAiNodeSetVec2(AtNode* node, const char* name, float x, float y);

AtVector RdoAiNodeGetVec(const AtNode* node, const char* name);
AtVector2 RdoAiNodeGetVec2(const AtNode* node, const char* name);

const char* RdoAiNodeGetStr(const AtNode* node, const char* name);

// Color
typedef AtRGB RdoAtRGB;

// Array
unsigned int RdoAiArrayGetNumElements(const AtArray* array);
unsigned char RdoAiArrayGetNumKeys(const AtArray* array);
unsigned char RdoAiArrayGetType(const AtArray* array);

AtVector RdoAiArrayGetVec(const AtArray* array, unsigned int i);
AtVector2 RdoAiArrayGetVec2(const AtArray* array, unsigned int i);
const char* RdoAiArrayGetStr(const AtArray* array, unsigned int i);

// Ray
#define RDO_AI_RAY_REFLECTED AI_RAY_ALL_REFLECT
#define RDO_AI_RAY_REFRACTED AI_RAY_ALL_TRANSMIT
#define RDO_AI_RAY_DIFFUSE AI_RAY_ALL_DIFFUSE
#define RDO_AI_RAY_GLOSSY AI_RAY_ALL_SPECULAR

// AtParamValue
const uint8_t& RdoBYTE(const AtParamValue* pValue);
const int& RdoINT(const AtParamValue* pValue);
const unsigned int& RdoUINT(const AtParamValue* pValue);
const bool& RdoBOOL(const AtParamValue* pValue);
const float& RdoFLT(const AtParamValue* pValue);
const AtRGB& RdoRGB(const AtParamValue* pValue);
const AtRGBA& RdoRGBA(const AtParamValue* pValue);
const AtVector& RdoVEC(const AtParamValue* pValue);
const AtVector2& RdoVEC2(const AtParamValue* pValue);
const char* RdoSTR(const AtParamValue* pValue);

// #else // ARNOLD 4

// #define RDO_WALTER_PROC "procedural"

// // Point/Vector
// #define RDO_AI_TYPE_VECTOR AI_TYPE_POINT
// #define RDO_AI_TYPE_VECTOR2 AI_TYPE_POINT2
// #define RDO_STR_TYPE_VECTOR2 "POINT2"
// #define RDO_STR_TYPE_VECTOR "POINT"

// typedef AtPoint RdoAtVector;
// typedef AtPoint2 RdoAtVector2;

// // Node
// void RdoAiNodeSetVec(AtNode* node, const char* name, float x, float y, float z);
// void RdoAiNodeSetVec2(AtNode* node, const char* name, float x, float y);

// AtPoint RdoAiNodeGetVec(const AtNode* node, const char* name);
// AtPoint2 RdoAiNodeGetVec2(const AtNode* node, const char* name);

// const char* RdoAiNodeGetStr(const AtNode* node, const char* name);

// // Color
// typedef AtColor RdoAtRGB;

// // Array
// unsigned int RdoAiArrayGetNumElements(const AtArray* array);
// unsigned char RdoAiArrayGetNumKeys(const AtArray* array);
// unsigned char RdoAiArrayGetType(const AtArray* array);

// AtPoint RdoAiArrayGetVec(const AtArray* array, unsigned int i);
// AtPoint2 RdoAiArrayGetVec2(const AtArray* array, unsigned int i);
// const char* RdoAiArrayGetStr(const AtArray* array, unsigned int i);

// // Ray
// #define RDO_AI_RAY_REFLECTED AI_RAY_REFLECTED
// #define RDO_AI_RAY_REFRACTED AI_RAY_REFRACTED
// #define RDO_AI_RAY_DIFFUSE AI_RAY_DIFFUSE
// #define RDO_AI_RAY_GLOSSY AI_RAY_GLOSSY

// // AtParamValue
// const AtByte& RdoBYTE(const AtParamValue* pValue);
// const int& RdoINT(const AtParamValue* pValue);
// const unsigned int& RdoUINT(const AtParamValue* pValue);
// const bool& RdoBOOL(const AtParamValue* pValue);
// const float& RdoFLT(const AtParamValue* pValue);
// const AtRGB& RdoRGB(const AtParamValue* pValue);
// const AtRGBA& RdoRGBA(const AtParamValue* pValue);
// const AtPoint& RdoVEC(const AtParamValue* pValue);
// const AtPoint2& RdoVEC2(const AtParamValue* pValue);
// const char* RdoSTR(const AtParamValue* pValue);

#endif  // AI_VERSION

#endif  // __RDO_ARNOLD_H__
