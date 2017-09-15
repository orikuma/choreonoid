#ifndef CNOID_AGXDYNAMICS_PLUGIN_AGX_BODY_H
#define CNOID_AGXDYNAMICS_PLUGIN_AGX_BODY_H

#include <cnoid/SimulatorItem>
#include <cnoid/BodyItem>
#include <cnoid/MeshExtractor>
#include <cnoid/BasicSensorSimulationHelper>
#include "AGXObjectFactory.h"
#include "AGXBodyPart.h"
#include "AGXBodyExtension.h"
#include "exportdecl.h"

namespace{
typedef std::function<bool(cnoid::AGXBody* agxBody)> AGXBodyExtensionFunc;
typedef std::map<std::string, AGXBodyExtensionFunc> AGXBodyExtensionFuncMap;
};

namespace cnoid{

typedef Eigen::Matrix<float, 3, 1> Vertex;
class AGXBodyPart;
typedef ref_ptr<AGXBodyPart> AGXBodyPartPtr;
typedef std::vector<AGXBodyPartPtr> AGXBodyPartPtrs;
class AGXBody;
typedef ref_ptr<AGXBody> AGXBodyPtr;
class AGXLink;
typedef ref_ptr<AGXLink> AGXLinkPtr;
typedef std::vector<AGXLinkPtr> AGXLinkPtrs;
typedef std::vector<AGXBodyExtensionPtr> AGXBodyExtensionPtrs;

static unsigned int generateUID(){
    static unsigned int i = 0;
    i++;
    return i;
}

class AGXLink : public Referenced
{
public:
    AGXLink(const LinkPtr link);
    AGXLink(const LinkPtr link, const AGXLinkPtr parent, const Vector3& parentOrigin, const AGXBodyPtr agxBody);
    void constructAGXLink();
    void setCollision(const bool& bOn);
    void setControlInputToAGX();
    void setLinkStateToAGX();
    void setLinkStateToCnoid();
    int getIndex() const;
    Vector3    getOrigin() const;
    LinkPtr    getOrgLink() const;
    AGXLinkPtr getAGXParentLink() const;
    agx::RigidBodyRef       getAGXRigidBody() const;
    agxCollide::GeometryRef getAGXGeometry() const;
    void                    setAGXConstraint(agx::ConstraintRef const constraint);
    agx::ConstraintRef      getAGXConstraint() const;
    std::string             getSelfCollisionGroupName() const;

private:
    LinkPtr     _orgLink;
    AGXLinkPtr  _agxParentLink;
    Vector3     _origin;
    agx::RigidBodyRef       _rigid;
    agxCollide::GeometryRef _geometry;
    agx::ConstraintRef      _constraint;
    std::string             _selfCollisionGroupName;

    agx::RigidBodyRef       createAGXRigidBody();
    agxCollide::GeometryRef createAGXGeometry();
    void createAGXShape();
    void detectPrimitiveShape(MeshExtractor* extractor, AGXTrimeshDesc& td);
    agx::ConstraintRef createAGXConstraint();
    void setTorqueToAGX();
    void setVelocityToAGX();
    void setPositionToAGX();
};

class CNOID_EXPORT AGXBody :  public SimulationBody
{
public:
    AGXBody(Body& orgBody);
    void initialize();
    void createBody();
    std::string getSelfCollisionGroupName() const;
    void setCollision(const bool& bOn);
    void setAGXMaterial(const int& index, const agx::MaterialRef& mat);
    void setControlInputToAGX();
    void setLinkStateToAGX();
    void setLinkStateToCnoid();
    bool hasForceSensors() const;
    bool hasGyroOrAccelerationSensors() const;
    void setSensor(const double& timeStep, const Vector3& gravity);
    void updateForceSensors();
    void updateGyroAndAccelerationSensors();
    int  numAGXLinks() const;
    void addAGXLink(AGXLinkPtr const agxLink);
    AGXLinkPtr getAGXLink(const int& index) const;
    AGXLinkPtr getAGXLink(const std::string& name) const;
    int numControllableLinks() const;
    void addControllableLink(AGXLinkPtr const agxLink);
    AGXLinkPtr getControllableLink(const int& index) const;
    agx::RigidBodyRef  getAGXRigidBody(const int& index) const;
    agx::ConstraintRef getAGXConstraint(const int& index) const;
    int numAGXBodyParts() const;
    void addAGXBodyPart(AGXBodyPartPtr const bp);
    AGXBodyPartPtr getAGXBodyPart(const int& index) const;
    bool addAGXBodyExtension(AGXBodyExtension* const extension);
    const AGXBodyExtensionPtrs& getAGXBodyExtensions() const;
    void callExtensionFunc();
    static void addAGXBodyExtensionAdditionalFunc(const std::string& typeName,
        std::function<bool(AGXBody* agxBody)> func);
    void updateAGXBodyExtensionFuncs();
private:
    std::string _selfCollisionGroupName;
    AGXLinkPtrs _agxLinks;
    AGXLinkPtrs _controllableLinks;
    AGXBodyPartPtrs _agxBodyParts;
    AGXBodyExtensionPtrs _agxBodyExtensions;
    BasicSensorSimulationHelper sensorHelper;
    AGXBodyExtensionFuncMap agxBodyExtensionFuncs;
    bool getAGXLinksFromInfo(const std::string& key, const bool& defaultValue, AGXLinkPtrs& agxLinks) const;
    void createExtraJoint();
    bool createContinuousTrack(AGXBody* agxBody);
    bool createAGXVehicleContinousTrack(AGXBody* agxBody);
};

}

#endif