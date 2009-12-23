#include <QtGui>

#include <QApplication>
#include <QMenuBar>
#include <QGroupBox>
#include <QGridLayout>
#include <QSlider>
#include <QTimer>
#include <QToolBar>
#include <QDockWidget>
#include <QVBoxLayout>
#include "mainwindow.h"
#include "logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(_RENDER_INTERVAL);
    /* Status Logger */
    printer = new CStatusPrinter();
    statusWidget = new CStatusWidget(printer);
    initLogger((void*)printer);

    /* Init Workspace */
    workspace = new QWorkspace(this);
    setCentralWidget(workspace);    

    /* Widgets */

    configwidget = new ConfigWidget();
    dockconfig = new ConfigDockWidget(this,configwidget);

    glwidget = new GLWidget(this,configwidget);
    glwidget->setWindowTitle(tr("Simulator"));
    glwidget->resize(512,512);

    robotwidget = new RobotWidget(this);
    /* Status Bar */
    fpslabel = new QLabel(this);
    cursorlabel = new QLabel(this);
    selectinglabel = new QLabel(this);
    statusBar()->addWidget(fpslabel);
    statusBar()->addWidget(cursorlabel);
    statusBar()->addWidget(selectinglabel);

    /* Menus */

    QMenu *fileMenu = new QMenu("&File");
    menuBar()->addMenu(fileMenu);
    QAction *exit = new QAction("E&xit", fileMenu);
    fileMenu->addAction(exit);

    QMenu *viewMenu = new QMenu("&View");
    menuBar()->addMenu(viewMenu);
    showsimulator = new QAction("&Simulator", viewMenu);
    showsimulator->setCheckable(true);
    showsimulator->setChecked(true);
    viewMenu->addAction(showsimulator);
    showconfig = new QAction("&Configuration", viewMenu);
    showconfig->setCheckable(true);
    showconfig->setChecked(true);
    viewMenu->addAction(showconfig);

    QMenu *simulatorMenu = new QMenu("&Simulator");
    menuBar()->addMenu(simulatorMenu);
    QMenu *cameraMenu = new QMenu("&Camera");
    QMenu *robotMenu = new QMenu("&Robot");
    QMenu *ballMenu = new QMenu("&Ball");
    simulatorMenu->addMenu(robotMenu);
    simulatorMenu->addMenu(ballMenu);
    cameraMenu->addAction(glwidget->changeCamModeAct);
    cameraMenu->addAction(glwidget->lockToRobotAct);
    cameraMenu->addAction(glwidget->lockToBallAct);

    ballMenu->addAction(tr("Put on Center"))->setShortcut(QKeySequence("-"));
    ballMenu->addAction(tr("Put on Corner 1"))->setShortcut(QKeySequence("Ctrl+1"));
    ballMenu->addAction(tr("Put on Corner 2"))->setShortcut(QKeySequence("Ctrl+2"));
    ballMenu->addAction(tr("Put on Corner 3"))->setShortcut(QKeySequence("Ctrl+3"));
    ballMenu->addAction(tr("Put on Corner 4"))->setShortcut(QKeySequence("Ctrl+4"));
    ballMenu->addAction(tr("Put on Penalty 1"))->setShortcut(QKeySequence("Alt+Ctrl+1"));
    ballMenu->addAction(tr("Put on Penalty 2"))->setShortcut(QKeySequence("Alt+Ctrl+2"));

    robotMenu->addMenu(glwidget->blueRobotsMenu);
    robotMenu->addMenu(glwidget->yellowRobotsMenu);

    fullScreenAct = new QAction(tr("&Full screen"),simulatorMenu);
    fullScreenAct->setShortcut(QKeySequence("F2"));
    fullScreenAct->setCheckable(true);
    fullScreenAct->setChecked(false);
    simulatorMenu->addAction(fullScreenAct);

    viewMenu->addAction(robotwidget->toggleViewAction());
    viewMenu->addMenu(cameraMenu);

    addDockWidget(Qt::LeftDockWidgetArea,dockconfig);
    addDockWidget(Qt::BottomDockWidgetArea, statusWidget);
    addDockWidget(Qt::LeftDockWidgetArea, robotwidget);
    workspace->addWindow(glwidget, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);

    QObject::connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    QObject::connect(exit, SIGNAL(triggered(bool)), this, SLOT(close()));
    QObject::connect(showsimulator, SIGNAL(triggered(bool)), this, SLOT(showHideSimulator(bool)));
    QObject::connect(showconfig, SIGNAL(triggered(bool)), this, SLOT(showHideConfig(bool)));
    QObject::connect(glwidget, SIGNAL(closeSignal(bool)), this, SLOT(showHideSimulator(bool)));    
    QObject::connect(dockconfig, SIGNAL(closeSignal(bool)), this, SLOT(showHideConfig(bool)));
    QObject::connect(glwidget, SIGNAL(selectedRobot()), this, SLOT(updateRobotLabel()));
    QObject::connect(robotwidget->robotCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(changeCurrentRobot()));
    QObject::connect(robotwidget->teamCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(changeCurrentTeam()));
    QObject::connect(robotwidget->resetBtn,SIGNAL(clicked()),glwidget,SLOT(resetCurrentRobot()));
    QObject::connect(robotwidget->locateBtn,SIGNAL(clicked()),glwidget,SLOT(moveCurrentRobot()));
    QObject::connect(robotwidget->onOffBtn,SIGNAL(clicked()),glwidget,SLOT(switchRobotOnOff()));
    QObject::connect(robotwidget->getPoseWidget->okBtn,SIGNAL(clicked()),this,SLOT(setCurrentRobotPosition()));
    QObject::connect(glwidget,SIGNAL(robotTurnedOnOff(int,bool)),robotwidget,SLOT(changeRobotOnOff(int,bool)));
    QObject::connect(ballMenu,SIGNAL(triggered(QAction*)),this,SLOT(ballMenuTriggered(QAction*)));
    QObject::connect(fullScreenAct,SIGNAL(triggered(bool)),this,SLOT(toggleFullScreen(bool)));
    QObject::connect(glwidget,SIGNAL(toggleFullScreen(bool)),this,SLOT(toggleFullScreen(bool)));
    //config related signals
    QObject::connect(configwidget->v_BALLMASS, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallMass()));
    QObject::connect(configwidget->v_CHASSISMASS, SIGNAL(wasEdited(VarType*)), this, SLOT(changeRobotMass()));
    QObject::connect(configwidget->v_KICKERMASS, SIGNAL(wasEdited(VarType*)), this, SLOT(changeKickerMass()));
    QObject::connect(configwidget->v_WHEELMASS, SIGNAL(wasEdited(VarType*)), this, SLOT(changeWheelMass()));    
    QObject::connect(configwidget->v_ballbounce, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallGroundSurface()));
    QObject::connect(configwidget->v_ballbouncevel, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallGroundSurface()));
    QObject::connect(configwidget->v_ballfriction, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallGroundSurface()));
    QObject::connect(configwidget->v_ballslip, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallGroundSurface()));
    QObject::connect(configwidget->v_ballangulardamp, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallDamping()));
    QObject::connect(configwidget->v_balllineardamp, SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallDamping()));
    QObject::connect(configwidget->v_Gravity,  SIGNAL(wasEdited(VarType*)), this, SLOT(changeGravity()));
    QObject::connect(configwidget->v_Kicker_Friction,  SIGNAL(wasEdited(VarType*)), this, SLOT(changeBallKickerSurface()));

    //geometry config vars
    QObject::connect(configwidget->v_BALLRADIUS, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_BOTTOMHEIGHT, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_CHASSISHEIGHT, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_CHASSISWIDTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_CHASSISLENGTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_KHEIGHT, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_KLENGTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_KWIDTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_KICKERHEIGHT, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_WHEELLENGTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v_WHEELRADIUS, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_LENGTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_MARGIN, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_PENALTY_LINE, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_PENALTY_RADIUS, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_RAD, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_FIELD_WIDTH, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));
    QObject::connect(configwidget->v__SSL_WALL_THICKNESS, SIGNAL(wasEdited(VarType*)), this, SLOT(alertStaticVars()));

    //network
    QObject::connect(configwidget->v_VisionMulticastAddr, SIGNAL(wasEdited(VarType*)), glwidget->ssl, SLOT(reconnectVisionSocket()));
    QObject::connect(configwidget->v_VisionMulticastPort, SIGNAL(wasEdited(VarType*)), glwidget->ssl, SLOT(reconnectVisionSocket()));
    QObject::connect(configwidget->v_BlueCommandListenPort, SIGNAL(wasEdited(VarType*)), glwidget->ssl, SLOT(reconnectBlueCommandSocket()));
    QObject::connect(configwidget->v_YellowCommandListenPort, SIGNAL(wasEdited(VarType*)), glwidget->ssl, SLOT(reconnectYellowCommandSocket()));

    timer->start();


    this->showMaximized();
    this->setWindowTitle("Parsian Simulator");

    robotwidget->teamCombo->setCurrentIndex(0);
    robotwidget->robotCombo->setCurrentIndex(0);
    robotwidget->setPicture(glwidget->ssl->robots[glwidget->Current_robot+glwidget->Current_team*5]->img);
    robotwidget->id = 0;
    scene = new QGraphicsScene(0,0,800,600);
}

MainWindow::~MainWindow()
{

}

void MainWindow::showHideConfig(bool v)
{
    if (v) {dockconfig->show();showconfig->setChecked(true);}
    else {dockconfig->hide();showconfig->setChecked(false);}
}

void MainWindow::showHideSimulator(bool v)
{
    if (v) {glwidget->show();showsimulator->setChecked(true);}
    else {glwidget->hide();showsimulator->setChecked(false);}
}

void MainWindow::changeCurrentRobot()
{
    glwidget->Current_robot=robotwidget->robotCombo->currentIndex();
    robotwidget->setPicture(glwidget->ssl->robots[glwidget->Current_robot+glwidget->Current_team*5]->img);
}

void MainWindow::changeCurrentTeam()
{
    glwidget->Current_team=robotwidget->teamCombo->currentIndex();
    robotwidget->setPicture(glwidget->ssl->robots[glwidget->Current_robot+glwidget->Current_team*5]->img);
}

void MainWindow::changeGravity()
{
    dWorldSetGravity (glwidget->ssl->p->world,0,0,-configwidget->Gravity());
}

QString floatToStr(float a)
{
    QString s;
    s.setNum(a,'f',3);
    if (a>=0) return QString('+') + s;
    return s;
}

void MainWindow::update()
{
    glwidget->updateGL();

    int R = robotIndex(glwidget->Current_robot,glwidget->Current_team);

    const dReal* vv = dBodyGetLinearVel(glwidget->ssl->robots[R]->chassis->body);
    static dVector3 lvv;
    dVector3 aa;
    aa[0]=(vv[0]-lvv[0])/configwidget->DeltaTime();
    aa[1]=(vv[1]-lvv[1])/configwidget->DeltaTime();
    aa[2]=(vv[2]-lvv[2])/configwidget->DeltaTime();
    robotwidget->vellabel->setText(QString::number(sqrt(vv[0]*vv[0]+vv[1]*vv[1]+vv[2]*vv[2]),'f',3));
    robotwidget->acclabel->setText(QString::number(sqrt(aa[0]*aa[0]+aa[1]*aa[1]+aa[2]*aa[2]),'f',3));
    lvv[0]=vv[0];
    lvv[1]=vv[1];
    lvv[2]=vv[2];

    fpslabel->setText(QString("Frame rate: %1 fps").arg(glwidget->getFPS()));
    if (glwidget->ssl->selected!=-1)
    {
        selectinglabel->setVisible(true);
        if (glwidget->ssl->selected==-2)
        {            
            selectinglabel->setText("Ball");
        }
        else
        {            
            int R = glwidget->ssl->selected%5;
            int T = glwidget->ssl->selected/5;
            if (T==0) selectinglabel->setText(QString("%1:Blue").arg(R));
            else selectinglabel->setText(QString("%1:Yellow").arg(R));
        }
    }
    else selectinglabel->setVisible(false);

    cursorlabel->setText(QString("Cursor: [X=%1;Y=%2;Z=%3]").arg(floatToStr(glwidget->ssl->cursor_x)).arg(floatToStr(glwidget->ssl->cursor_y)).arg(floatToStr(glwidget->ssl->cursor_z)));
    statusWidget->update();
}

void MainWindow::updateRobotLabel()
{
    robotwidget->teamCombo->setCurrentIndex(glwidget->Current_team);
    robotwidget->robotCombo->setCurrentIndex(glwidget->Current_robot);
    robotwidget->id = robotIndex(robotwidget->robotCombo->currentIndex(),robotwidget->teamCombo->currentIndex());
}


void MainWindow::changeBallMass()
{
    glwidget->ssl->ball->setMass(configwidget->BALLMASS());
}

void MainWindow::changeRobotMass()
{    
    for (int i=0;i<10;i++)
    {
        glwidget->ssl->robots[i]->chassis->setMass(configwidget->CHASSISMASS()*0.99f);
        glwidget->ssl->robots[i]->dummy->setMass(configwidget->CHASSISMASS()*0.01f);
    }
}

void MainWindow::changeKickerMass()
{
    for (int i=0;i<10;i++)
    {
        glwidget->ssl->robots[i]->kicker->box->setMass(configwidget->KICKERMASS());
    }
}

void MainWindow::changeWheelMass()
{
    for (int i=0;i<10;i++)
    {
        for (int j=0;j<4;j++)
        {
            glwidget->ssl->robots[i]->wheels[j]->cyl->setMass(configwidget->WHEELMASS());
        }
    }    
}

void MainWindow::changeBallGroundSurface()
{
    PSurface* ballwithwall = glwidget->ssl->p->findSurface(glwidget->ssl->ball,glwidget->ssl->ground);    
    ballwithwall->surface.mode = dContactBounce | dContactApprox1 | dContactSlip1 | dContactSlip2;
    ballwithwall->surface.mu = fric(configwidget->ballfriction());
    ballwithwall->surface.bounce = configwidget->ballbounce();
    ballwithwall->surface.bounce_vel = configwidget->ballbouncevel();
    ballwithwall->surface.slip1 = configwidget->ballslip();
    ballwithwall->surface.slip2 = configwidget->ballslip();
}

void MainWindow::changeBallKickerSurface()
{
    PSurface* ballwithkicker = glwidget->ssl->p->findSurface(glwidget->ssl->ball,glwidget->ssl->robots[0]->kicker->box);
    ballwithkicker->surface.mu = configwidget->Kicker_Friction();
}

void MainWindow::changeBallDamping()
{
    dBodySetLinearDampingThreshold(glwidget->ssl->ball->body,0.001);
    dBodySetLinearDamping(glwidget->ssl->ball->body,configwidget->balllineardamp());
    dBodySetAngularDampingThreshold(glwidget->ssl->ball->body,0.001);
    dBodySetAngularDamping(glwidget->ssl->ball->body,configwidget->ballangulardamp());
}

void MainWindow::alertStaticVars()
{    
    logStatus("You must restart application to see the changes of geometry parameters",QColor("orange"));
}

void MainWindow::ballMenuTriggered(QAction* act)
{
    float l = configwidget->_SSL_FIELD_LENGTH()/2000.0f;
    float w = configwidget->_SSL_FIELD_WIDTH()/2000.0f;
    float p = l - configwidget->_SSL_FIELD_PENALTY_POINT()/1000.f;
    if (act->text()==tr("Put on Center")) glwidget->putBall(0,0);
    else if (act->text()==tr("Put on Corner 1")) glwidget->putBall(-l,-w);
    else if (act->text()==tr("Put on Corner 2")) glwidget->putBall(-l, w);
    else if (act->text()==tr("Put on Corner 3")) glwidget->putBall( l,-w);
    else if (act->text()==tr("Put on Corner 4")) glwidget->putBall( l, w);
    else if (act->text()==tr("Put on Penalty 1")) glwidget->putBall( p, 0);
    else if (act->text()==tr("Put on Penalty 2")) glwidget->putBall(-p, 0);
}

void MainWindow::toggleFullScreen(bool a)
{
    if (a)
    {
        view = new GLWidgetGraphicsView(scene,glwidget);
        lastSize = glwidget->size();        
        view->setViewport(glwidget);
        view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        view->setFrameStyle(0);
        view->showFullScreen();
        view->setFocus();        
        glwidget->fullScreen = true;
        fullScreenAct->setChecked(true);
    }
    else {
        view->close();        
        workspace->addWindow(glwidget, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
        glwidget->show();
        glwidget->resize(lastSize);
        glwidget->fullScreen = false;
        glwidget->setFocusPolicy(Qt::StrongFocus);
        glwidget->setFocus();
        fullScreenAct->setChecked(false);
    }
}

void MainWindow::setCurrentRobotPosition()
{
    int i = robotIndex(glwidget->Current_robot,glwidget->Current_team);
    bool ok1=false,ok2=false,ok3=false;
    float x = robotwidget->getPoseWidget->x->text().toFloat(&ok1);
    float y = robotwidget->getPoseWidget->y->text().toFloat(&ok2);
    float a = robotwidget->getPoseWidget->a->text().toFloat(&ok3);
    if (!ok1) {logStatus("Invalid float for x",QColor("red"));return;}
    if (!ok2) {logStatus("Invalid float for y",QColor("red"));return;}
    if (!ok3) {logStatus("Invalid float for angle",QColor("red"));return;}
    glwidget->ssl->robots[i]->setXY(x,y);
    glwidget->ssl->robots[i]->setDir(a);
    robotwidget->getPoseWidget->close();
}
