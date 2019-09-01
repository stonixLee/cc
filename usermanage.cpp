#include "usermanage.h"
#include "ui_usermanage.h"
#include "search.h"
#include <QUuid>
#include <QMessageBox>
#include <QWidget>

void UserManage::DatabaseConnecting()//连接到数据库
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
        return;
    }
}

void UserManage::openTable()//打开表，将tableView中填充数据以显示
{
    saveFlag = false;//刚开始未选中行，所以设为false
    theModel->clear();
    theModel->setTable("t_usertable");
    theModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
    theModel->setSort(theModel->fieldIndex("User_Privilege"),Qt::DescendingOrder);
    if(!theModel->select())
    {
        QMessageBox::critical(this,"错误","打开数据表错误，错误信息\n"+theModel->
                              lastError().text(),QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    theModel->setHeaderData(theModel->fieldIndex("User_Name"),Qt::Horizontal,"姓名");
    theModel->setHeaderData(theModel->fieldIndex("User_LoginName"),Qt::Horizontal,"登录名");
    theModel->setHeaderData(theModel->fieldIndex("User_Prof"),Qt::Horizontal,"专业");
    theModel->setHeaderData(theModel->fieldIndex("User_Posts"),Qt::Horizontal,"岗位");
    theModel->setHeaderData(theModel->fieldIndex("User_Privilege"),Qt::Horizontal,"访问权限");

    //创建界面组件与数据模型的字段之间的数据映射
    dataMapper->clearMapping();
    itemSelection->clear();
    itemSelection->setModel(theModel);
    dataMapper->setModel(theModel);//设置数据模型
    dataMapper->setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

    connect(itemSelection,SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this,SLOT(on_currentRowChanged(QModelIndex,QModelIndex)));

    ui->tableView->setModel(theModel);
    ui->tableView->setSelectionModel(itemSelection);
    ui->tableView->setColumnHidden(theModel->fieldIndex("User_id"),true);//隐藏id字段
    ui->tableView->setColumnHidden(theModel->fieldIndex("User_Password"),true);//隐藏密码字段
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableView->setSelectionMode(QAbstractItemView::MultiSelection);//可多选表格行
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);//以行为单位选中表格中的数据

    QStringList profession = {"专业A","专业B","专业C","专业D"};
    ui->profComboBox->clearEditText();
    ui->profComboBox->addItems(profession);
    QStringList post = {"岗位1","岗位2","岗位3","岗位4"};
    ui->postComboBox->clearEditText();
    ui->postComboBox->addItems(post);
    QStringList privilege = {"管理员","普通用户"};
    ui->privilegeComboBox->clearEditText();
    ui->privilegeComboBox->addItems(privilege);

    ui->passwordEdit->clear();
    ui->nameEdit->clear();
    ui->loginNameEdit->clear();
}

void UserManage::addUser()
{
    QString name = ui->nameEdit->text();
    QString loginName = ui->loginNameEdit->text();
    QString password = ui->passwordEdit->text();
    QString prof = ui->profComboBox->currentText();
    QString post = ui->postComboBox->currentText();
    QString privilege = ui->privilegeComboBox->currentText();
    QSqlQuery query(DB);
    query.prepare("insert into t_usertable(User_id,User_Name,User_LoginName,"
                  "User_Password,User_Prof,User_Posts,User_Privilege) values(?,?,?,?,?,?,?)");
    query.addBindValue(QUuid::createUuid().toString().mid(1,UUID_LENGTH));
    query.addBindValue(name);
    query.addBindValue(loginName);
    query.addBindValue(password);
    query.addBindValue(prof);
    query.addBindValue(post);
    query.addBindValue(privilege);
    query.exec();
    QMessageBox::information(this,"提示","该用户信息已上传数据库",QMessageBox::Ok,QMessageBox::NoButton);
}

void UserManage::createMenu()
{


    contextMenul->exec(QCursor::pos());
}

UserManage::UserManage(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UserManage)
{
    ui->setupUi(this);
    //上下文菜单以及action建立
    actDelete = new QAction("删除");
    connect(actDelete,SIGNAL(triggered(bool)),
            this,SLOT(on_actDelete_triggered(bool)));
    ui->tableView->addAction(actDelete);
    ui->tableView->setContextMenuPolicy(Qt::ActionsContextMenu);
    //状态栏连接部分
    curStatusDesc=new QLabel("与仿真服务之间的连接：");
    curState=new QLabel("ok");
    curImitateTaskDesc=new QLabel("当前仿真任务");
    curImitateTask=new QLabel("XXX工程");
    ImitateProjectCombobox = new QComboBox();
    //更新操作
    actRefresh = new QAction(QIcon(":/png/png/refresh.png"),"更新");
    connect(actRefresh,SIGNAL(triggered(bool)),
            this,SLOT(on_actRefresh_triggered(bool)));

    ui->toolBar->addWidget(ImitateProjectCombobox);
    ui->toolBar->addAction(actRefresh);
    ui->statusbar->addWidget(curStatusDesc);
    ui->statusbar->addWidget(curState,1);
    ui->statusbar->addWidget(curImitateTaskDesc);
    ui->statusbar->addWidget(curImitateTask,1);
    DatabaseConnecting();
    //创建模型，选择模型，mapper
    theModel= new QSqlTableModel(this,DB);
    itemSelection = new QItemSelectionModel();
    dataMapper= new QDataWidgetMapper();
    openTable();
    //创建删除活动
//    connect(ui->tableView,SIGNAL(),this,
//            SLOT(on_actDelete_triggered(bool)));
//    createMenu();
}

UserManage::~UserManage()
{
    delete ui;
}

void UserManage::on_currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{

    if(previous.data().toString()!="")//第一次不执行，鼠标选择空白处不执行
    {

        //界面组件与theModel的具体字段之间的联系
        dataMapper->addMapping(ui->nameEdit,theModel->fieldIndex("User_Name"));
        dataMapper->addMapping(ui->loginNameEdit,theModel->fieldIndex("User_LoginName"));
        dataMapper->addMapping(ui->passwordEdit,theModel->fieldIndex("User_Password"));
        dataMapper->addMapping(ui->postComboBox,theModel->fieldIndex("User_Posts"));
        dataMapper->addMapping(ui->profComboBox,theModel->fieldIndex("User_Prof"));
        dataMapper->addMapping(ui->privilegeComboBox,theModel->fieldIndex("User_Privilege"));
            dataMapper->setCurrentIndex(current.row()); //更新数据映射的行号
            saveFlag=true;//选中行后可修改
    }
    if (!current.isValid())
        return;
}

void UserManage::on_saveButton_clicked()//修改按钮的实现
{

    if(ui->nameEdit->text()=="" || ui->loginNameEdit->text()=="" || ui->passwordEdit->text()==""){
        QMessageBox::warning(this,"warning","不允许修改为空值！",
                             QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    QModelIndexList list = itemSelection->selectedIndexes();//获取选中的行
      int  row =list.count();
    if (row==1)
        saveFlag=true;
    else
        saveFlag=false;
    if(saveFlag)
    {
        QSqlQuery modify(DB);
        modify.prepare("update t_usertable set User_Name='"+
                       ui->nameEdit->text()+"', User_LoginName='"+
                       ui->loginNameEdit->text()+"', User_Password='"+
                       ui->passwordEdit->text()+"', User_Posts='"+
                       ui->postComboBox->currentText()+"', User_Prof='"+
                       ui->profComboBox->currentText()+"', User_Privilege='"+
                       ui->privilegeComboBox->currentText()+"' where User_id='"+
                       itemSelection->selectedIndexes().at(0).data().toString()+"'");
        modify.exec();
            QMessageBox::information(this,"提示","该用户信息已在数据库中更改！",
                                     QMessageBox::Ok,QMessageBox::NoButton);
     }
    else
        QMessageBox::warning(this,"warning","请选择要修改的行！",
                             QMessageBox::Ok,QMessageBox::NoButton);
}

void UserManage::on_resetButton_clicked()
{
    saveFlag = false;
    dataMapper->clearMapping();
    ui->nameEdit->setText("");
    ui->loginNameEdit->setText("");
    ui->passwordEdit->setText("");
    ui->postComboBox->itemText(0);
    ui->privilegeComboBox->itemText(0);
    ui->profComboBox->itemText(0);
}

void UserManage::on_addButton_clicked()
{
    if(ui->nameEdit->text()=="" || ui->loginNameEdit->text()=="" || ui->passwordEdit->text()==""){
        QMessageBox::warning(this,"warning","请将用户信息填写完整！",
                             QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    if(!saveFlag)
        addUser();
    else
        QMessageBox::warning(this,"warning","请重置后再操作！",
                             QMessageBox::Ok,QMessageBox::NoButton);
    ui->passwordEdit->clear();
    ui->nameEdit->clear();
    ui->loginNameEdit->clear();
}

void UserManage::on_searchButton_clicked()
{
    Search dlgFindByField;
    ui->tableView->clearSelection();
    Qt::WindowFlags winflag=dlgFindByField.windowFlags();
    dlgFindByField.setWindowFlags(winflag | Qt::MSWindowsFixedSizeDialogHint);
    int ret = dlgFindByField.exec();//模态
    if(ret==QDialog::Accepted)
    {//开始查找
        QString findName=dlgFindByField.getName();
        QString findPost=dlgFindByField.getPost();
        QString findProf=dlgFindByField.getProf();
        if(findName!="")//如果输入名字就按名字查找
        {
            bool flag=false;
            for(int i=0;i<theModel->rowCount();i++)
            {
                if(findName==theModel->record(i).value("User_Name").toString())
                {
                    ui->tableView->selectRow(i);
                    flag=true;
                    break;
                }
            }
            if(!flag)
                QMessageBox::warning(this,"warning","查找失败，未找到"+findName,
                                     QMessageBox::Ok,QMessageBox::NoButton);
        }
        else if(findPost!="")//如果名字为空，岗位不为空按照岗位查找
        {
            bool flag=false;
            for(int i=0;i<theModel->rowCount();i++)
            {
                if(findPost==theModel->record(i).value("User_Posts").toString())
                {
                    ui->tableView->selectRow(i);
                    flag=true;
                }
            }
            if(!flag)
                QMessageBox::warning(this,"warning","查找失败，未找到‘"+findPost+"’岗位",
                                     QMessageBox::Ok,QMessageBox::NoButton);
        }
        else if(findProf!="")
        {
            bool flag=false;
            for(int i=0;i<theModel->rowCount();i++)
            {
                if(findProf==theModel->record(i).value("User_Prof").toString())
                {
                    ui->tableView->selectRow(i);
                    flag=true;
                }
            }
            if(!flag)
                QMessageBox::warning(this,"warning","查找失败，未找到‘"+findProf+"’专业的人员",
                                     QMessageBox::Ok,QMessageBox::NoButton);
        }
    }

}

void UserManage::on_actDelete_triggered(bool isTrue)
{
    Q_UNUSED(isTrue);
    if(itemSelection->selectedIndexes().count()==0){
        QMessageBox::warning(this,"warning","请选择要删除的行",
                             QMessageBox::Ok,QMessageBox::NoButton);
        return;
    }
    for(int i=0;i<itemSelection->selectedRows().count();i++)
    {
        QSqlQuery deleteQuery(DB);
        //获取要删除的人的ID并删除
        QString id=itemSelection->selectedRows(0).at(i).data().toString();
        deleteQuery.prepare("DELETE FROM t_usertable WHERE User_id='"+id+"'");
        deleteQuery.exec();
    }

    openTable();

}

void UserManage::on_actRefresh_triggered(bool isTrue)
{
    Q_UNUSED(isTrue);
        qDebug()<<"entered the refresh trigger";
        openTable();

}


