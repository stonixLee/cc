#ifndef USERMANAGE_H
#define USERMANAGE_H

#include <QMainWindow>
#include<QLabel>
#include<QComboBox>
#include<QAction>
#include <QtSql>
#include <QDataWidgetMapper>
#include <QMenu>
#define UUID_LENGTH 36
namespace Ui {
class UserManage;
}

class UserManage : public QMainWindow
{
    Q_OBJECT

private:
    bool saveFlag;//判断是修改数据后保存还是新增数据
    QLabel *curStatusDesc;//当前仿真服务之间的连接状态的描述
    QLabel *curState;//连接状态
    QLabel *curImitateTaskDesc;//当前仿真任务描述
    QLabel *curImitateTask;//当前仿真任务
    QComboBox *ImitateProjectCombobox;//状态栏上的下拉列表
    QAction *actRefresh;//状态栏更新按钮

    QSqlDatabase DB;    //数据库连接
    QSqlTableModel *theModel;//用户表模型
    QItemSelectionModel *itemSelection;//选择模型
    QDataWidgetMapper   *dataMapper; //数据映射(表格->lineEdit)

    QAction *actDelete;//删除活动
    QMenu *contextMenul;//上下文菜单
    void DatabaseConnecting();//数据库连接
    void openTable();//打开数据表
    void addUser();//添加用户写进数据库
    void createMenu();//创建上下文菜单栏
public:
    explicit UserManage(QWidget *parent = 0);
    ~UserManage();

private:
    Ui::UserManage *ui;
private slots:
    void on_currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void on_saveButton_clicked();
    void on_resetButton_clicked();
    void on_addButton_clicked();
    void on_searchButton_clicked();
    void on_actDelete_triggered(bool isTrue);
    void on_actRefresh_triggered(bool isTrue);
};

#endif // USERMANAGE_H
