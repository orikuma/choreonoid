/**
   @author Shin'ichiro Nakaoka
*/

#include "App.h"
#include "ExtensionManager.h"
#include "DescriptionDialog.h"
#include "OptionManager.h"
#include <QApplication>

namespace cnoid {

class AppImpl : public QObject
{
    Q_OBJECT

    friend class App;
    friend class View;
        
    AppImpl(App* self, int& argc, char**& argv);
    ~AppImpl();
        
    App* self;
    QApplication* qapplication;
    int& argc;
    char**& argv;
    ExtensionManager* ext;
    std::string appName;
    std::string vendorName;
    DescriptionDialog* descriptionDialog;
    bool doQuit;
        
    void initialize(const char* appName, const char* vendorName, const QIcon& icon, const char* pluginPathList);
    int exec();
    void onSigOptionsParsed(boost::program_options::variables_map& v);
    bool processCommandLineOptions();
    virtual bool eventFilter(QObject* watched, QEvent* event);
        
    void showInformationDialog();
    void onOpenGLVSyncToggled(bool on);
    static void clearFocusView();
                             
private Q_SLOTS:
    void onFocusChanged(QWidget* old, QWidget* now);
};
}
