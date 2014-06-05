/**
   @author Shin'ichiro Nakaoka
*/

#include "SceneView.h"
#include "ViewManager.h"
#include "ItemTreeView.h"
#include "Separator.h"
#include "RootItem.h"
#include "Button.h"
#include <cnoid/SceneProvider>
#include <boost/bind.hpp>
#include <list>
#include "gettext.h"

using namespace std;
using namespace cnoid;

namespace {
vector<SceneView*> instances;
boost::signals::connection sigItemAddedConnection;
}

namespace cnoid {

class SceneViewImpl
{
public:
    SceneView* self;
    SceneWidget* sceneWidget;
    SceneWidgetRoot* sceneRoot;

    struct SceneInfo {
        Item* item;
        SceneProvider* provider;
        SgNodePtr scene;
        bool isShown;
        boost::signals::connection sigDetachedFromRootConnection;
        boost::signals::connection sigCheckToggledConnection;
        SceneInfo(Item* item, SceneProvider* provider)
            : item(item), provider(provider) {
            isShown = false;
        }
        ~SceneInfo(){
            sigDetachedFromRootConnection.disconnect();
            sigCheckToggledConnection.disconnect();
        }
    };

    list<SceneInfo> sceneInfos;

    ItemTreeView* itemTreeView;
    CheckBox dedicatedCheckCheck;
    int dedicatedCheckId;
        
    SceneViewImpl(SceneView* self);
    ~SceneViewImpl();
    void onSceneProviderItemAdded(Item* item, SceneProvider* provider);
    void onSceneProviderItemDetachedFromRoot(list<SceneInfo>::iterator infoIter);
    void showScene(list<SceneInfo>::iterator infoIter, bool show);
    void onDedicatedCheckToggled(bool on);
    bool storeState(Archive& archive);
    bool restoreState(const Archive& archive);
};
}


SceneView* SceneView::instance()
{
    if(instances.empty()){
        return 0;
    }
    return instances.front();
}


void SceneView::initializeClass(ExtensionManager* ext)
{
    if(instances.empty()){
        ext->viewManager().registerClass<SceneView>(
            "SceneView", N_("Scene"), ViewManager::MULTI_DEFAULT);

        sigItemAddedConnection =
            RootItem::mainInstance()->sigItemAdded().connect(
                boost::bind(&SceneView::onItemAdded, _1));
    }
}


namespace {
void finalizeClass()
{
    sigItemAddedConnection.disconnect();
}
}


SceneView::SceneView()
{
    impl = new SceneViewImpl(this);
}


SceneViewImpl::SceneViewImpl(SceneView* self)
    : self(self)
{
    self->setDefaultLayoutArea(View::RIGHT);
    
    sceneWidget = new SceneWidget;
    sceneRoot = sceneWidget->sceneRoot();

    QVBoxLayout* vbox = new QVBoxLayout;
    vbox->addWidget(sceneWidget);
    self->setLayout(vbox);

    vbox = sceneWidget->setupDialogVBox();
    vbox->addWidget(new HSeparator);
    QHBoxLayout* hbox = new QHBoxLayout;
    dedicatedCheckCheck.setText(_("Use dedicated item tree view checks to select the target items"));
    dedicatedCheckCheck.sigToggled().connect(
        boost::bind(&SceneViewImpl::onDedicatedCheckToggled, this, _1));
    hbox->addWidget(&dedicatedCheckCheck);
    hbox->addStretch();
    vbox->addLayout(hbox);

    itemTreeView = ItemTreeView::instance();

    dedicatedCheckId = -1;

    if(!instances.empty()){
        SceneViewImpl* mainImpl = instances.front()->impl;
        list<SceneInfo>::iterator p;
        for(p = mainImpl->sceneInfos.begin(); p != mainImpl->sceneInfos.end(); ++p){
            SceneInfo& info = *p;
            onSceneProviderItemAdded(info.item, info.provider);
        }
    }
    
    instances.push_back(self);
}


SceneView::~SceneView()
{
    delete impl;
}


SceneViewImpl::~SceneViewImpl()
{
    if(dedicatedCheckId >= 0){
        itemTreeView->releaseCheckColumn(dedicatedCheckId);
    }

    instances.erase(std::find(instances.begin(), instances.end(), self));

    if(instances.empty()){
        finalizeClass();
    }
}
    

void SceneView::onActivated()
{

}


void SceneView::onDeactivated()
{

}


SceneWidget* SceneView::sceneWidget()
{
    return impl->sceneWidget;
}
    

SceneWidgetRoot* SceneView::sceneRoot()
{
    return impl->sceneRoot;
}


void SceneView::onItemAdded(Item* item)
{
    if(SceneProvider* provider = dynamic_cast<SceneProvider*>(item)){
        for(size_t i=0; i < instances.size(); ++i){
            instances[i]->impl->onSceneProviderItemAdded(item, provider);
        }
    }
}


void SceneViewImpl::onSceneProviderItemAdded(Item* item, SceneProvider* provider)
{
    sceneInfos.push_back(SceneInfo(item, provider));
    list<SceneInfo>::iterator infoIter = sceneInfos.end();
    --infoIter;
    SceneInfo& info = *infoIter;
        
    info.sigDetachedFromRootConnection =
        item->sigDetachedFromRoot().connect(
            boost::bind(&SceneViewImpl::onSceneProviderItemDetachedFromRoot, this, infoIter));

    int checkId = dedicatedCheckCheck.isChecked() ? dedicatedCheckId : 0;
        
    info.sigCheckToggledConnection =
        itemTreeView->sigCheckToggled(item, checkId).connect(
            boost::bind(&SceneViewImpl::showScene, this, infoIter, _1));
        
    if(itemTreeView->isItemChecked(item, checkId)){
        showScene(infoIter, true);
    }
}


void SceneViewImpl::onSceneProviderItemDetachedFromRoot(list<SceneInfo>::iterator infoIter)
{
    showScene(infoIter, false);
    sceneInfos.erase(infoIter);
}


void SceneViewImpl::showScene(list<SceneInfo>::iterator infoIter, bool show)
{
    if(infoIter->isShown && !show){
        if(infoIter->scene){
            sceneRoot->removeChild(infoIter->scene, true);
        }
        infoIter->isShown = false;
        
    } else if(!infoIter->isShown && show){
        if(!infoIter->scene){
            infoIter->scene = infoIter->provider->scene();
        }
        if(infoIter->scene){
            sceneRoot->addChild(infoIter->scene, true);
            infoIter->isShown = true;
        }
    }
}


void SceneViewImpl::onDedicatedCheckToggled(bool on)
{
    int checkId = 0;
    
    if(on){
        if(dedicatedCheckId < 0){
            dedicatedCheckId = itemTreeView->addCheckColumn();
            itemTreeView->setCheckColumnToolTip(dedicatedCheckId, self->windowTitle());
        }
        itemTreeView->showCheckColumn(dedicatedCheckId, true);
        checkId = dedicatedCheckId;

    } else {
        if(dedicatedCheckId >= 0){
            itemTreeView->showCheckColumn(dedicatedCheckId, false);
        }
    }

    for(list<SceneInfo>::iterator p = sceneInfos.begin(); p != sceneInfos.end(); ++p){
        p->sigCheckToggledConnection.disconnect();
        p->sigCheckToggledConnection =
            itemTreeView->sigCheckToggled(p->item, checkId).connect(
                boost::bind(&SceneViewImpl::showScene, this, p, _1));
        
        showScene(p, itemTreeView->isItemChecked(p->item, checkId));
    }
}


QWidget* SceneView::indicatorOnInfoBar()
{
    return impl->sceneWidget->indicator();
}


bool SceneView::storeState(Archive& archive)
{
    return impl->storeState(archive);
}


bool SceneViewImpl::storeState(Archive& archive)
{
    bool result = true;
    result &= sceneWidget->storeState(archive);
    archive.write("dedicatedItemTreeViewChecks", dedicatedCheckCheck.isChecked());
    if(dedicatedCheckCheck.isChecked()){
        itemTreeView->storeCheckColumnState(dedicatedCheckId, archive);
    }
    return result;
}


bool SceneView::restoreState(const Archive& archive)
{
    return impl->restoreState(archive);
}


bool SceneViewImpl::restoreState(const Archive& archive)
{
    bool result = true;
    result &= sceneWidget->restoreState(archive);
    dedicatedCheckCheck.setChecked(archive.get("dedicatedItemTreeViewChecks", dedicatedCheckCheck.isChecked()));
    if(dedicatedCheckCheck.isChecked()){
        result &= itemTreeView->restoreCheckColumnState(dedicatedCheckId, archive);
    }
    return result;
}
