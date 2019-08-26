#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qcompressthread.h"
#include "quploadthread.h"
#include    <QFileDialog>
#include    <QMessageBox>
#include    <QUuid>
#include    <JlCompress.h>
#include    <QVector>
#include    <QHostInfo>
#include    <QFile>
#include    <qnetwork.h>
#include    <QToolTip>
#include <QToolTip>
//QToolTip::showText(QCursor::pos(),text);
void MainWindow::initUI()
{
    setSysCombobox();
    initTree();
    inParaTab = new QSqlTableModel(this, DB);
    outParaTab = new QSqlTableModel(this, DB);
    outParaTab->setHeaderData(0,Qt::Horizontal,"参数名称");
    outParaTab->setHeaderData(1,Qt::Horizontal,"参数类型");
    outParaTab->setHeaderData(2,Qt::Horizontal,"参数默认值");
    outParaTab->setHeaderData(3,Qt::Horizontal,"参数输出方式");
    ui->searchBtn->setIcon(QIcon(":/png/png/search.png"));
    ui->refreshBtn->setIcon(QIcon(":/png/png/refresh.png"));
    //底部状态栏

    ui->statusBar->setStyleSheet(QString("QStatusBar::item{border:0px}"));
    ui->statusBar->addWidget(communication,0);
    ui->statusBar->addWidget(connectState,1);
    ui->statusBar->addWidget(DBconnect,0);
    ui->statusBar->addWidget(DBconnectState,1);

    ui->statusBar->addWidget(curPersonLab,0);
    ui->statusBar->addWidget(person,1);
}

void MainWindow::setSysCombobox()
{
    QSqlQuery ProjectName(DB);
    ProjectName.exec("select Project_Name from t_simulationproject");
    while(ProjectName.next())
    {
        ui->HypotaxisCombobox->addItem(ProjectName.value(0).toString());
    }
    ProName = ui->HypotaxisCombobox->currentText();
}

void MainWindow::initTree()//初始化树目录
{
    treeModel = new QStandardItemModel();
    treeSelection = new QItemSelectionModel(treeModel);
    QString sParent = "select Project_id,Project_Name from t_simulationproject";
    QString sChildren;
    QSqlQuery qParent(DB),qChildren(DB);
    QStringList parent;
    QVector<QVector<QString>> set;
    qParent.exec(sParent);
    while(qParent.next())
    {
       parent.append(qParent.value(1).toString());
       sChildren="select Model_Name from t_projectmodels where Project_id='"
               +qParent.value(0).toString()+"'";
       qChildren.exec(sChildren);
       QVector<QString> subset;     //存放来自数据库中同一系统的模型
       while(qChildren.next())
           subset.append(qChildren.value(0).toString());
       set.append(subset);
    }

    for (int i=0;i<parent.length();i++)
    {
        QString str = parent.at(i);
        QStandardItem *item = new QStandardItem(str);
        item->setData(QIcon(":/png/png/Open.png"),Qt::DecorationRole);
        treeModel->appendRow(item);
        for (int j=0;j<set.at(i).length();j++)
        {
            QString s = set[i][j];
            QStandardItem *childItem = new QStandardItem(s);
            childItem->setData(QIcon(":/png/png/New.png"),Qt::DecorationRole);
            treeModel->item(i)->appendRow(childItem);
        }
    }
    ui->treeView->setModel(treeModel);
    ui->treeView->setHeaderHidden(true);
    ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);//关闭目录的可编辑状态
}

void MainWindow::on_TableListChanged(const QModelIndex &index)
{
    tabModel = new QSqlQueryModel(this);
    inPara = new QSqlQuery(DB);
    outPara = new QSqlQuery(DB);
    if (index.parent().data().toString()=="")//是目录节点
    {
        tabModel->setQuery("SELECT Model_Name,Model_Version,Model_Pos,Model_Path,Model_Date,"
                           "User_Name FROM t_usertable,t_projectmodels WHERE t_projectmodels.User_id"
                           " = t_usertable.User_id and Project_id = (SELECT Project_id FROM t_simulationproject "
                           "WHERE Project_Name = '"+index.data().toString()+"')");//获取数据库中：模型名字，版本，位置，路径，上传时间，上传人字段
        if (tabModel->lastError().isValid())
        {
            QMessageBox::critical(this,"错误",
                                  "数据表查询错误\n"+tabModel->lastError().text(),
                                  QMessageBox::Ok,QMessageBox::NoButton);
            return;
        }
        /*
         *对时间进行排序代码。。.
         */
    }
    else           /*如果是模型结点设置成点击模型后对应的表格行突出显示并跳转到该目录下*/
    {
        //找到模型的父目录对应的列表
        tabModel->setQuery("SELECT Model_Name,Model_Version,Model_Pos,Model_Path,Model_Date,"
                           "User_Name FROM t_usertable,t_projectmodels WHERE t_projectmodels.User_id"
                           " = t_usertable.User_id and Project_id = (SELECT Project_id FROM t_simulationproject "
                           "WHERE Project_Name = '"+index.parent().data().toString()+"')");

        //通过当前选取的模型名字获取模型ID
        inPara->exec("SELECT Model_id FROM t_projectmodels WHERE Model_Name = '"
                     +index.data().toString()+"'");
        inPara->next();
        QString modelId = inPara->value(0).toString();
        //获取与模型ID匹配的参数信息
        inParaTab->setTable("t_modelparameters");
        inParaTab->setEditStrategy(QSqlTableModel::OnManualSubmit);
        inParaTab->setFilter(QObject::tr("Model_id='%1'").arg(modelId));
        outParaTab->setTable("t_modelparameters");//输出参数表格模型
        outParaTab->setEditStrategy(QSqlTableModel::OnManualSubmit);
        outParaTab->setFilter(QObject::tr("Model_id='%1'").arg(modelId));
    }
    ui->modelListView->setSelectionMode(QAbstractItemView::SingleSelection);//单选表格
    ui->modelListView->setSelectionBehavior(QAbstractItemView::SelectRows);//单选行
    ui->modelListView->setStyleSheet("selection-background-color:rgb(255,209,128)");//高亮选中行的颜色为橘色
    //设置模型表头
    tabModel->setHeaderData(0,Qt::Horizontal,"模型名称");
    tabModel->setHeaderData(1,Qt::Horizontal,"版本号");
    tabModel->setHeaderData(2,Qt::Horizontal,"存储位置");
    tabModel->setHeaderData(3,Qt::Horizontal,"存储目录");
    tabModel->setHeaderData(4,Qt::Horizontal,"上传时间");
    tabModel->setHeaderData(5,Qt::Horizontal,"上传人");

    selectionTableModel=new QItemSelectionModel(tabModel);
   /*
    connect(selectionTableModel,SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this,SLOT(on_currentRowChanged(QModelIndex,QModelIndex)));


    connect(...)实现点击Item显示全内容
    connect（。。。）实现鼠标放到Item上显示全内容
   槽函数未实现，处理点击行对应的参数列表变换*/
    ui->modelListView->setModel(tabModel);
    ui->modelListView->setSelectionModel(selectionTableModel);
    ui->modelListView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    if(index.parent().data().toString()!="")
        ui->modelListView->selectRow(index.row());//获取选中当前行高亮显示
    //设置输入参数表头
    inParaTab->removeColumns(0,2);//移除参数ID和模型ID字段
    inParaTab->removeColumns(2,2);//移除除参数名称和参数类型外的字段
    inParaTab->setHeaderData(inParaTab->fieldIndex("Parameters_Name"),Qt::Horizontal,"参数名称");
    inParaTab->setHeaderData(inParaTab->fieldIndex("Parameters_Type"),Qt::Horizontal,"参数类型");
    //缺少的字段。。。...
    inParaTab->setHeaderData(2,Qt::Horizontal,"参数默认值");
    inParaTab->setHeaderData(3,Qt::Horizontal,"参数输入方式");

    inParaTab->select();
    ui->inParamTabView->setModel(inParaTab);
    ui->inParamTabView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //设置输出参数表头
    outParaTab->removeColumns(0,2);
    outParaTab->removeColumns(2,2);
    outParaTab->setHeaderData(outParaTab->fieldIndex("Parameters_Name"),Qt::Horizontal,"参数名称");
    outParaTab->setHeaderData(outParaTab->fieldIndex("Parameters_Type"),Qt::Horizontal,"参数类型");
    outParaTab->setHeaderData(2,Qt::Horizontal,"参数默认值");
    outParaTab->setHeaderData(3,Qt::Horizontal,"参数输出方式");
    outParaTab->select();
    ui->outParamTabView->setModel(outParaTab);
    ui->outParamTabView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
}

void MainWindow::DatabaseConnecting()
{
    DB =QSqlDatabase::addDatabase("QMYSQL");
    DB.setDatabaseName("Simu");
    DB.setHostName("localhost");
    DB.setPassword("root");
    DB.setPort(3306);
    DB.setUserName("root");
    if (!DB.open())   //打开数据库
    {
        QMessageBox::warning(this, "错误", "打开数据库失败",
                                 QMessageBox::Ok,QMessageBox::NoButton);
        DBconnectState->setText("no");
        return;
    }
    DBconnectState->setText("ok");//数据库连接状态ok
}

QString MainWindow::getLocalIP()
{//获取本机的ip地址
    QString hostName=QHostInfo::localHostName();//本地主机名
    QHostInfo   hostInfo=QHostInfo::fromName(hostName);
    QString   localIP="";

    QList<QHostAddress> addList=hostInfo.addresses();//

    if (!addList.isEmpty())
    for (int i=0;i<addList.count();i++)
    {
        QHostAddress aHost=addList.at(i);
        if (QAbstractSocket::IPv4Protocol==aHost.protocol())
        {
            localIP=aHost.toString();
            break;
        }
    }
    return localIP;
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    communication = new QLabel("通信连接：");
    connectState = new QLabel("disconnect");
    DBconnect = new QLabel("数据库连接：");
    DBconnectState = new QLabel("no");
    curPersonLab = new QLabel("当前操作人员：");
    person = new QLabel("***");
    DatabaseConnecting();
    initUI();
    tcpClient = new QTcpSocket(this);
    upLoad = new QUpLoadThread();
    connect(&comp,SIGNAL(started()),this,SLOT(on_started()));//开始压缩数据前对按钮操作
    connect(&comp,SIGNAL(finished()),this,SLOT(on_finished()));//结束压缩前将按钮激活
    //实现点击树目录显示目录中的节点信息到模型列表
    connect(ui->treeView,SIGNAL(clicked(QModelIndex)),this,
            SLOT(on_TableListChanged(QModelIndex)));
    connect(tcpClient,SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            this,SLOT(onSocketStateChange(QAbstractSocket::SocketState)));
    connect(tcpClient,SIGNAL(connected()),this,SLOT(onClientConnected()));
    connect(tcpClient,SIGNAL(disconnected()),this,SLOT(onClientDisconnected()));

}

MainWindow::~MainWindow()
{
    delete ui;

}

void MainWindow::on_SearchMF_Btn_clicked()
{
    QString curPath = "E:/";
    MFPath = QFileDialog::getExistingDirectory(this,"选择描述模型的目录",
                                               curPath,QFileDialog::ShowDirsOnly);
    if(MFPath.isEmpty())
        return;
    ui->MF_Edit->setText(MFPath);

}

void MainWindow::on_XML_Btn_clicked()
{
    XMLfullName = QFileDialog::getOpenFileName(this,"选择XML文件","","所有文件(*.*);;XML文件(*.xml)");
    if(XMLfullName.isEmpty())
        return;
    ui->XML_Edit->setText(XMLfullName);
}


void MainWindow::on_ModelSubmitBtn_clicked()//上传到数据库、压缩打包等
{
    MF = getFinalFolderName(MFPath);
    QString XML = getFinalFolderName(XMLfullName);
    //判断输入行是否有为空
    if(MF.isEmpty())
       {
        QMessageBox::warning(this,"warning 提示","请输入文件名称",QMessageBox::Ok,QMessageBox::NoButton);
        return;
        }
    if(XML.isEmpty()){
        QMessageBox::warning(this,"warning 提示","请输入XML模型描述文件",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    if(aDir.isEmpty()){
        QMessageBox::warning(this,"warning 提示","请输入文件上传路径",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    if(version.isEmpty()){
        QMessageBox::warning(this,"warning 提示","请输入版本号",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    if(ui->CompressCheckBox->isChecked() && connectState->text()!="connected")
    {
        QMessageBox::warning(this,"warning 提示","上传失败，未连接上服务器！",QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    //将MF,XML,modelID,aDir,version信息写进数据库


    if(!isContainItem())//更新t_simulationproject表
    {
        if(ProName.isEmpty())
            QMessageBox::warning(this,"warning 提示","设置所属系统！");
        ui->HypotaxisCombobox->addItem(ProName);//在下拉列表中添加子系统
        QSqlQuery addProject(DB);
        addProject.prepare("insert into t_simulationproject(Project_id,Project_Number,"
                            "Project_Name,Project_Modelfile) values(?,?,?,?)");
        addProject.addBindValue(QUuid::createUuid().toString().mid(1,UUID_LENGTH));
        addProject.addBindValue("1");
        addProject.addBindValue(ProName);
        addProject.addBindValue("文件2");//要进行修改*********************
        addProject.exec();
    }
    QSqlQuery addModel(DB);
    addModel.prepare("insert into t_projectmodels(Model_id,Project_id,Model_Name,"
                     "Model_Version,Model_Date,Model_Pos,Model_Path,User_id) values(?,?,?,?,?,?,?,?)");
    addModel.addBindValue(QUuid::createUuid().toString().mid(1,UUID_LENGTH));
    addModel.addBindValue(fromProNameToProID(ProName));
    addModel.addBindValue(modelName);
    addModel.addBindValue(version);
    addModel.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    addModel.addBindValue(serverIP);
    addModel.addBindValue(aDir);//存放路径
    addModel.addBindValue("1be11abf-f751-425e-8ef0-2b4f285110e8");//gggg操作员的id
    addModel.exec();
    if(ui->CompressCheckBox->isChecked())
    {
        if(connectState->text()=="connected")//如果连接上服务器
        {
        comp.setData(aDir,MF,MFPath);
        comp.start();
        }
    }
}



QString MainWindow::getFinalFolderName(const QString &fullPathName)//从一个完整的目录名称里获得最后的文件名称
{
    int cnt=fullPathName.length();
    int i = fullPathName.lastIndexOf("/");
    QString str = fullPathName.right(cnt-1-i);
    return str;
}

bool MainWindow::isContainItem()//判断是否在所属系统的下拉列表中含有ProName
{
    int num = ui->HypotaxisCombobox->count();
    int i;
    for(i=0;i<num;i++)
        if(ui->HypotaxisCombobox->itemText(i)==ProName)
          {
            break;
         }
    return i==num ? false:true;
}

QString MainWindow::fromProNameToProID(const QString &Proname)//从项目名获取项目ID
{
    QSqlQuery query(DB);
    QString select = "select Project_id from t_simulationproject where Project_Name = '"+Proname+"'";
    query.exec(select);
    query.next();
    return query.value(0).toString();
}

void MainWindow::on_ModelNameEdit_cursorPositionChanged(int arg1, int arg2)
{
    Q_UNUSED(arg1);
    Q_UNUSED(arg2);
    modelName = ui->ModelNameEdit->text();
}

void MainWindow::on_HypotaxisCombobox_currentIndexChanged(const QString &arg1)
{
    ProName=ui->HypotaxisCombobox->currentText();
}

void MainWindow::on_MF_PosBtn_clicked()//还需完善:先选择服务器ip再选择存放位置
{
    Dialog *dlgGetIP =new Dialog(this);
    QStringList ipList;
    ipList.append(getLocalIP());
    ipList.append("192.168.0.1");//无用的IP
    ipList.append("192.168.0.2");//无用的IP
    dlgGetIP->setIpList(ipList);
    Qt::WindowFlags flags = dlgGetIP->windowFlags();
    dlgGetIP->setWindowFlags(flags | Qt::MSWindowsFixedSizeDialogHint);
    int ret = dlgGetIP->exec();//模态对话框
    if(ret=QDialog::Accepted)//确定按钮被按下
    {
        if(dlgGetIP->Ip!="")
            serverIP = dlgGetIP->Ip;
        else
            {
            QMessageBox::warning(this,"warning","请选择服务器IP地址",QMessageBox::Ok,QMessageBox::NoButton);
            serverIP="";
            }
    }
    delete dlgGetIP;
//判断客户端连接状态
    if(connectState->text()=="connected"){ //连接到服务器
        tcpClient->disconnectFromHost();//先断开连接
        tcpClient->connectToHost(serverIP,PORT);   //连接到新的服务器
    }
    else if(connectState->text()=="disconnected" && serverIP != ""){
        tcpClient->connectToHost(serverIP,PORT);
    }
    else if(serverIP!=""){
//        tcpClient->disconnectFromHost();
        tcpClient->abort();//立即终止连接，但是要丢弃write缓冲区的包
        tcpClient->connectToHost(serverIP,PORT);
    }
    QString assignPath = "F:/sss/compressfile";
//    aDir = QFileDialog::getExistingDirectory(this,"选择一个目录",assignPath,QFileDialog::ShowDirsOnly);
    aDir = assignPath;
    ui->MF_SubmitPosEdit->setText(serverIP+" "+aDir);

}

void MainWindow::on_ModelVersionEdit_editingFinished()
{
    version=ui->ModelVersionEdit->text();
}

void MainWindow::on_HypotaxisCombobox_editTextChanged(const QString &arg1)
{
    ProName = ui->HypotaxisCombobox->currentText();
}

void MainWindow::on_started()
{
    ui->ModelSubmitBtn->setText("压缩上传中...");
    ui->ModelSubmitBtn->setEnabled(false);
}

void MainWindow::on_finished()
{
    upLoad->setData(aDir,MF,MFPath,serverIP);
    upLoad->start();
    ui->ModelSubmitBtn->setText("模型上传");
    ui->ModelSubmitBtn->setEnabled(true);
    initTree();

}

void MainWindow::on_searchBtn_clicked()
{
    QString name = ui->topLineEdit->text();
    QSqlQuery select(DB);
    QModelIndex parentIndex,childIndex;
    QStringList dir;//存放有相同文件名的树目录父节点
    select.exec("SELECT Project_Name FROM t_simulationproject WHERE "
                "Project_id IN(SELECT Project_id FROM t_projectmodels "
                "WHERE `Model_Name`='"+name+"')");
    while(select.next())
    {
        dir.append(select.value(0).toString());
    }
    if(dir.count()==1)//判断模型名字没有重名的情况，这时模型的目录只有一个
    {
        for(int i=0;i<treeModel->rowCount();i++){//依次获取父节点信息，找到所要查找的结点的父节点
            parentIndex = treeModel->index(i,0);
            if (parentIndex.data().toString() == dir.at(0)){//找到父节点
                ui->treeView->setExpanded(parentIndex,true);//扩展父项
                for(int j=0;j<treeModel->item(parentIndex.row(),0)->rowCount();j++){//从父节点中获得子节点
                    childIndex = treeModel->index(j,0,parentIndex);
                    if(childIndex.data().toString() == name){
                        ui->treeView->setCurrentIndex(childIndex);
                    }
                }

            }

        }

    }

}

void MainWindow::onSocketStateChange(QAbstractSocket::SocketState socketState)
{//stateChange()信号槽函数
    switch(socketState)
    {
    case QAbstractSocket::UnconnectedState:
        connectState->setText("UnconnectedState");
        break;
    case QAbstractSocket::HostLookupState:
        connectState->setText("HostLookupState");
        break;
    case QAbstractSocket::ConnectingState:
        connectState->setText("ConnectingState");
        break;

    case QAbstractSocket::ConnectedState:
        connectState->setText("ConnectedState");
        break;

    case QAbstractSocket::BoundState:
        connectState->setText("BoundState");
        break;

    case QAbstractSocket::ClosingState:
        connectState->setText("ClosingState");
        break;

    case QAbstractSocket::ListeningState:
        connectState->setText("ListeningState");
    }
}
void MainWindow::onClientConnected()
{//客户端接入时
    connectState->setText("connected");
}

void MainWindow::onClientDisconnected()
{//客户端断开连接时
    connectState->setText("disconnected");
}

void MainWindow::on_modelListView_clicked(const QModelIndex &index)
{
    if(!index.isValid())
        return;
    QString text = index.data().toString();
    if(text.isEmpty())
        return;
    QToolTip::showText(QCursor::pos(),text);
}
