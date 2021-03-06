/**
   \file
   \author Shin'ichiro Nakaoka
*/

#include "GrxUIMenuView.h"
#include <cnoid/ViewManager>
#include <cnoid/MainWindow>
#include <cnoid/MessageView>
#include <cnoid/Dialog>
#include <cnoid/Button>
#include <cnoid/LineEdit>
#include <cnoid/PythonExecutor>
#include <QBoxLayout>
#include <QStackedLayout>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QMessageBox>
#include <QEventLoop>
#include "gettext.h"

using namespace std;
using namespace boost;
using namespace cnoid;

namespace {

python::object cancelExceptionType;

class FuncParamEntry : public LineEdit
{
public:
    int typeChar;
    std::string marker;

    FuncParamEntry(int typeChar, const std::string& marker)
        : typeChar(typeChar),
          marker(marker)
        {

        }
        
    virtual QSize sizeHint() const {
        QString sizeText;
        if(typeChar == 'D'){
            sizeText = "123.56";
        } else if(typeChar == 'T'){
            sizeText = "Text Input";
        }
        QSize s = LineEdit::sizeHint();
        return QSize(fontMetrics().boundingRect(sizeText).width(), s.height());
    }

    std::string text() const {
        return LineEdit::text().toStdString();
    }
};
        

class FuncButtonBox : public QWidget
{
public:
    PushButton button;
    string funcString;
    vector<FuncParamEntry*> entries;
        
    FuncButtonBox(const string& label, const string& funcString)
        {
            QHBoxLayout* hbox = new QHBoxLayout();
            setLayout(hbox);

            button.setText(label.c_str());
            hbox->addWidget(&button);

            int pos = 0;
            while(true){
                pos = funcString.find('#', pos);
                if(pos == std::string::npos){
                    break;
                }
                if((pos + 1) == funcString.size()){
                    break;
                }
                string marker = funcString.substr(pos, 2);
                ++pos;
                int typeChar = toupper(funcString[pos]);
                
                if(typeChar == 'D' || typeChar == 'T'){
                    FuncParamEntry* entry = new FuncParamEntry(typeChar, marker);
                    hbox->addWidget(entry);
                    entries.push_back(entry);
                } else {
                    break;
                }
            }
            
            this->funcString = funcString;
        }

    string label() const {
        return button.text().toStdString();
    }
};


class MenuWidget : public QWidget
{
public:
    GrxUIMenuViewImpl* view;

    QStackedLayout pageStack;
    QStackedLayout barStack;

    ToolButton sequenceModeButton;
    ToolButton quitButton1;
        
    ToolButton regularModeButton;
    ToolButton prevPageButton;
    QLabel pageIndexLabel;
    ToolButton nextPageButton;
    CheckBox sequentialCheck;
    ToolButton retryButton;
    ToolButton quitButton2;

    bool isLocalSequentialMode;
    int currentSequencePageIndex;
    int currentButtonPage;
    int currentButton;
    int lastIndexInPage;
    vector<vector<QWidget*> > buttonList;
    PythonExecutor pythonExecutor;

    MenuWidget(const python::list& menu, bool isLocalSequentialMode, bool doBackgroundExecution, GrxUIMenuViewImpl* view);
    void createPages(const python::list& menu);
    void addSection(const python::list& section);
    void onButtonClicked(int indexInPage, FuncButtonBox* box);
    void onScriptFinished();
    void moveNext();
    void moveToNextButton();
    bool isSequenceMode();
    void showSequencePages();
    void showRegularPage();
    void moveSequentialPage(int direction);
    void setCurrentSequentialPage(int index);
    void onSequentialCheckToggled(bool on);
    void onRetryClicked();
    void onQuitClicked();
};
}

namespace cnoid {

class GrxUIMenuViewImpl
{
public:
    GrxUIMenuView* self;
    QVBoxLayout vbox;
    QLabel noMenuLabel;
    MenuWidget* menuWidget;
    int menuButtonsBlockCount;

    GrxUIMenuViewImpl(GrxUIMenuView* self);
    ~GrxUIMenuViewImpl();
    void clearMenu(bool doDelete, bool showLabel);
    void setMenu(const python::list& menu, bool isLocalSequentialMode, bool doBackgroundExecution);
    void blockMenuButtons(bool on);
};
}


namespace {

struct MenuButtonBlock {
    GrxUIMenuViewImpl* impl;
    MenuButtonBlock(GrxUIMenuViewImpl* impl) : impl(impl) {
        impl->blockMenuButtons(true);
    }
    ~MenuButtonBlock(){
        impl->blockMenuButtons(false);
    }
};
}


void GrxUIMenuView::initializeClass(ExtensionManager* ext)
{
    ext->viewManager().registerClass<GrxUIMenuView>(
        "GrxUIMenuView", N_("GrxUI Menu"), ViewManager::SINGLE_OPTIONAL);
}


GrxUIMenuView* GrxUIMenuView::instance()
{
    return ViewManager::getOrCreateView<GrxUIMenuView>();
}


void GrxUIMenuView::setCancelExceptionType(boost::python::object exceptionType)
{
    cancelExceptionType = exceptionType;
}


GrxUIMenuView::GrxUIMenuView()
{
    impl = new GrxUIMenuViewImpl(this);
}


GrxUIMenuViewImpl::GrxUIMenuViewImpl(GrxUIMenuView* self)
    : self(self)
{
    self->setDefaultLayoutArea(View::CENTER);

    self->setLayout(&vbox);
    noMenuLabel.setText(_("There is no menu now."));
    noMenuLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    vbox.addWidget(&noMenuLabel, 0, Qt::AlignHCenter);
    
    menuWidget = 0;
    menuButtonsBlockCount = 0;

    clearMenu(false, true);
}


GrxUIMenuView::~GrxUIMenuView()
{
    delete impl;
}


GrxUIMenuViewImpl::~GrxUIMenuViewImpl()
{

}


void GrxUIMenuViewImpl::clearMenu(bool doDelete, bool showLabel)
{
    if(menuWidget){
        vbox.removeWidget(menuWidget);
        if(doDelete){
            delete menuWidget;
        }
        menuWidget = 0;
    }

    if(showLabel){
        noMenuLabel.show();
    } else {
        noMenuLabel.hide();
    }
}


void GrxUIMenuView::setMenu(const python::list& menu, bool isLocalSequentialMode, bool doBackgroundExecution)
{
    impl->setMenu(menu, isLocalSequentialMode, doBackgroundExecution);
}


void GrxUIMenuViewImpl::setMenu(const python::list& menu, bool isLocalSequentialMode, bool doBackgroundExecution)
{
    clearMenu(true, false);

    menuWidget = new MenuWidget(menu, isLocalSequentialMode, doBackgroundExecution, this);
    vbox.addWidget(menuWidget);
    noMenuLabel.hide();

    MessageView::instance()->notify(_("A new menu has been set to the GrxUI Menu View."));
}


void GrxUIMenuViewImpl::blockMenuButtons(bool on)
{
    if(on){
        ++menuButtonsBlockCount;
    } else{
        --menuButtonsBlockCount;
    }
    if(menuWidget){
        bool enabled = (menuButtonsBlockCount <= 0);
        QStackedLayout& pageStack = menuWidget->pageStack;
        const int n = pageStack.count();
        for(int i=0; i < n; ++i){
            pageStack.widget(i)->setEnabled(enabled);
        }
    }
}


/*
  This function is defined here instead of PyGrxUI.cpp so that the message translations can be edited with one file
*/
QMessageBox::StandardButton GrxUIMenuView::waitInputSelect(const std::string& message)
{
    QMessageBox box(MainWindow::instance());
    box.setWindowTitle(_("Wait input select"));
    box.setText(message.c_str());
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Yes);
    box.setModal(false);
    box.show();

    MenuButtonBlock block(instance()->impl);
    QEventLoop eventLoop;
    connect(&box, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return (QMessageBox::StandardButton)box.result();
}


/*
  This function is defined here instead of PyGrxUI.cpp so that the message translations can be edited with one file
*/
bool GrxUIMenuView::waitInputConfirm(const std::string& message)
{
    QMessageBox box(MainWindow::instance());
    box.setWindowTitle(_("Wait input confirm"));
    box.setText(message.c_str());
    box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    box.setDefaultButton(QMessageBox::Ok);
    box.setModal(false);
    box.show();

    MenuButtonBlock block(instance()->impl);
    QEventLoop eventLoop;
    connect(&box, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return (box.result() == QMessageBox::Ok);
}


/*
  This function is defined here instead of PyGrxUI.cpp so that the message translations can be edited with one file
*/
std::string GrxUIMenuView::waitInputMessage(const std::string& message)
{
    Dialog dialog;
    dialog.setWindowTitle(_("Wait input message"));
    QVBoxLayout* vbox = new QVBoxLayout();
    dialog.setLayout(vbox);

    vbox->addWidget(new QLabel(message.c_str()));
    
    LineEdit* lineEdit = new LineEdit();
    connect(lineEdit, SIGNAL(returnPressed()), &dialog, SLOT(accept()));
    vbox->addWidget(lineEdit);

    PushButton* okButton = new PushButton(_("&OK"));
    okButton->setDefault(true);
    connect(okButton, SIGNAL(clicked()), &dialog, SLOT(accept()));
    vbox->addWidget(okButton);
    vbox->addStretch();
    
    dialog.show();

    MenuButtonBlock block(instance()->impl);
    QEventLoop eventLoop;
    connect(&dialog, SIGNAL(finished(int)), &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return lineEdit->string();
}


MenuWidget::MenuWidget
(const python::list& menu, bool isLocalSequentialMode, bool doBackgroundExecution, GrxUIMenuViewImpl* view)
    : view(view)
{
    QVBoxLayout* vbox = new QVBoxLayout();
    
    QFrame* barFrame = new QFrame();
    barFrame->setFrameShape(QFrame::StyledPanel);
    barFrame->setLayout(&barStack);
            
    vbox->addWidget(barFrame);
    vbox->addLayout(&pageStack, 1);

    QHBoxLayout* hbox;

    QWidget* regularBar = new QWidget(this);
    regularBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    hbox = new QHBoxLayout();

    this->isLocalSequentialMode = isLocalSequentialMode;
    sequenceModeButton.setText("V");
    sequenceModeButton.setToolTip(_("Show buttons executed sequentially"));
    sequenceModeButton.sigClicked().connect(boost::bind(&MenuWidget::showSequencePages, this));
    hbox->addWidget(&sequenceModeButton);
    hbox->addStretch();
            
    hbox->addWidget(new QLabel(_("Command Always Enabled")));
    hbox->addStretch();
            
    quitButton1.setText("X");
    quitButton1.setToolTip(_("Quit this menu"));
    quitButton1.sigClicked().connect(boost::bind(&MenuWidget::onQuitClicked, this));
    hbox->addWidget(&quitButton1);
            
    regularBar->setLayout(hbox);
    barStack.addWidget(regularBar);
            
    QWidget* sequenceBar = new QWidget(this);
    sequenceBar->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
            
    hbox = new QHBoxLayout();

    regularModeButton.setText("^");
    regularModeButton.setToolTip(_("Show buttons always enabled"));
    regularModeButton.sigClicked().connect(boost::bind(&MenuWidget::showRegularPage, this));
    hbox->addWidget(&regularModeButton);
            
    hbox->addStretch();

    prevPageButton.setText("<");
    prevPageButton.setToolTip(_("Show previous menu"));
    prevPageButton.sigClicked().connect(boost::bind(&MenuWidget::moveSequentialPage, this, -1));
    hbox->addWidget(&prevPageButton);
            
    hbox->addWidget(&pageIndexLabel);
            
    nextPageButton.setText(">");
    nextPageButton.setToolTip(_("Show next menu"));
    nextPageButton.sigClicked().connect(boost::bind(&MenuWidget::moveSequentialPage, this, +1));
    hbox->addWidget(&nextPageButton);
            
    sequentialCheck.setText(_("sequential"));
    sequentialCheck.setToolTip(_("Enable sequential execution"));
    sequentialCheck.setChecked(true);
    sequentialCheck.sigToggled().connect(boost::bind(&MenuWidget::onSequentialCheckToggled, this, _1));
    hbox->addWidget(&sequentialCheck);

    hbox->addStretch();
            
    retryButton.setText("<-|");
    retryButton.setToolTip(_("Retry from first"));
    retryButton.sigClicked().connect(boost::bind(&MenuWidget::onRetryClicked, this));
    hbox->addWidget(&retryButton);
            
    quitButton2.setText("X");
    quitButton2.setToolTip(_("Quit this menu"));
    quitButton2.sigClicked().connect(boost::bind(&MenuWidget::onQuitClicked, this));
    hbox->addWidget(&quitButton2);
            
    sequenceBar->setLayout(hbox);
    barStack.addWidget(sequenceBar);
            
    setLayout(vbox);

    createPages(menu);

    pythonExecutor.setBackgroundMode(doBackgroundExecution);
    pythonExecutor.sigFinished().connect(boost::bind(&MenuWidget::onScriptFinished, this));

    currentButton = 0;
    currentButtonPage = 0;
    currentSequencePageIndex = 0;

    bool hasSequencePages = false;
    if(buttonList.size() >= 2){
        for(int i=1; i < buttonList.size(); ++i){
            if(!buttonList[i].empty()){
                hasSequencePages = true;
                break;
            }
        }
    }
    if(hasSequencePages){
        showSequencePages();
    } else {
        showRegularPage();
    }
}


void MenuWidget::createPages(const python::list& menu)
{
    int numSections = python::len(menu);
    for(int i=0; i < numSections; ++i){
        const python::list section = python::extract<python::list>(menu[i]);
        addSection(section);
    }

    while(pageStack.count() < 2){
        addSection(python::list());
    }
}


void MenuWidget::addSection(const python::list& section)
{                        
    QWidget* page = new QWidget();
    page->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    QVBoxLayout* vbox = new QVBoxLayout();
    page->setLayout(vbox);

    buttonList.push_back(vector<QWidget*>());
    vector<QWidget*>& buttons = buttonList.back();

    int numItems = python::len(section) / 2;

    for(int j=0; j < numItems; ++j){
        // extract a pair of elements
        const string label = python::extract<string>(section[j*2]);
        const string function = python::extract<string>(section[j*2+1]);

        if(function == "#label"){
            vbox->addWidget(new QLabel(label.c_str()), 0, Qt::AlignCenter);

        } else if(label == "#monitor"){

        } else {
            FuncButtonBox* box = new FuncButtonBox(label, function);
            box->button.sigClicked().connect(boost::bind(&MenuWidget::onButtonClicked, this, buttons.size(), box));
            vbox->addWidget(box, 0, Qt::AlignCenter);
            buttons.push_back(box);
        }
    }

    vbox->addStretch();

    QScrollArea* area = new QScrollArea();
    area->setWidgetResizable(true);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setAlignment(Qt::AlignHCenter);
    area->setWidget(page);
    pageStack.addWidget(area);
}


void MenuWidget::onButtonClicked(int indexInPage, FuncButtonBox* box)
{
    string code = box->funcString;

    int pos = 0;
    for(int i=0; i < box->entries.size(); ++i){
        const FuncParamEntry* entry = box->entries[i];
        pos = code.find(entry->marker, pos);
        if(pos != string::npos){
            string value = entry->text();
            code.replace(pos, entry->marker.length(), value);
            pos += value.length();
        }
    }

    lastIndexInPage = indexInPage;

    if(pythonExecutor.state() == PythonExecutor::RUNNING_BACKGROUND){
        bool doTermination =
            showConfirmDialog(
                _("Python Script Termination"),
                str(format(_("The Python script assigned to button \"%1%\" is still running in the background thread. "
                             "Do you want to terminate it to execute the command you clicked?"))
                    % box->label()));
        if(doTermination){
            if(!pythonExecutor.terminate()){
                showWarningDialog(_("The script cannot be terminated and the command you clicked cannot be executed."));
                return;
            }
        }
    }

    pythonExecutor.execCode(code);
}


void MenuWidget::onScriptFinished()
{
    bool result = true;
    
    if(pythonExecutor.hasException()){
        PyGILock lock;
        if(pythonExecutor.exceptionType() == cancelExceptionType){
            showWarningDialog(_("The script has been cancelled."));
        } else {
            MessageView::instance()->putln(pythonExecutor.exceptionText());
        }
        result = false;
    }

    if(result){
        moveNext();
    }
}


void MenuWidget::moveNext()
{
    if(isSequenceMode() && sequentialCheck.isChecked()){
        if(isLocalSequentialMode){
            moveToNextButton();
        } else if(lastIndexInPage == 0){
            if(currentSequencePageIndex + 1 < buttonList.size() - 1){
                setCurrentSequentialPage(currentSequencePageIndex + 1);
            }
        }
    }
}


void MenuWidget::moveToNextButton()
{
    vector<QWidget*> buttons = buttonList[currentSequencePageIndex+1];
    if(currentButton < buttons.size() - 1){
        ++currentButton;
        setCurrentSequentialPage(currentSequencePageIndex);
    } else if(currentSequencePageIndex < buttonList.size() - 1){
        currentButton = 0;
        ++currentButtonPage; 
        setCurrentSequentialPage(currentSequencePageIndex + 1);
    }
}


bool MenuWidget::isSequenceMode() {
    return (barStack.currentIndex() == 1);
}


void MenuWidget::showSequencePages()
{
    barStack.setCurrentIndex(1);
    setCurrentSequentialPage(currentSequencePageIndex);
}


void MenuWidget::showRegularPage()
{
    barStack.setCurrentIndex(0);
    pageStack.setCurrentIndex(0);
}


void MenuWidget::moveSequentialPage(int direction)
{
    const int lastPage = pageStack.count() - 2;
    const int index = std::max(0, std::min(currentSequencePageIndex + direction, lastPage));
    setCurrentSequentialPage(index);
}


void MenuWidget::setCurrentSequentialPage(int index)
{
    pageStack.setCurrentIndex(index + 1);

    if(isLocalSequentialMode){
        const bool hasCurrentButton = (index == currentButtonPage);
        vector<QWidget*>& buttons = buttonList[index + 1];
        for(int i=0; i < buttons.size(); ++i){
            buttons[i]->setEnabled(
                !sequentialCheck.isChecked() ||(hasCurrentButton && (i == currentButton)));
        }
    }
            
    prevPageButton.setEnabled(!sequentialCheck.isChecked() && index > 0);
    const int numPages = pageStack.count() - 1;
    nextPageButton.setEnabled(!sequentialCheck.isChecked() && index < numPages - 1);
    currentSequencePageIndex = index;
    pageIndexLabel.setText(QString("%1/%2").arg(index + 1).arg(numPages));
}


void MenuWidget::onSequentialCheckToggled(bool on)
{
    setCurrentSequentialPage(currentSequencePageIndex);
}


void MenuWidget::onRetryClicked()
{
    currentButton = 0;
    currentButtonPage = 0;
    setCurrentSequentialPage(0);
}


void MenuWidget::onQuitClicked()
{
    view->clearMenu(false, true);
    deleteLater();
}
