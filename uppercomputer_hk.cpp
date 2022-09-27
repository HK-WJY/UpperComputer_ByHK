#include "uppercomputer_hk.h"
#include "ui_uppercomputer_hk.h"


/*窗口界面的构造函数*/
UpperComputer_HK::UpperComputer_HK(QWidget *parent):QMainWindow(parent),ui(new Ui::UpperComputer_HK)
{
    ui->setupUi(this);

    /*初始化UI界面和相关变脸*/
    UiConfig();

    /*创建定时器并且定时扫描串口*/
    timerforport = new QTimer();
    timerforport->start(50);
    connect(timerforport,&QTimer::timeout,this,&UpperComputer_HK::reflashinfo);

    /*创建串口并且初始化串口参数配置*/
    this->myport = new QSerialPort();
    old_Comnum = 0;now_Comnum = 0;
    SettingsConfig();


    /*初始化图表*/
    Chart_Config();

    /*创建自动发送的定时器*/
    timerforsend = new QTimer();

    setWindowIcon(QIcon("HK.icon"));
}

/*窗口界面的析构函数，用来释放空间*/
UpperComputer_HK::~UpperComputer_HK()
{
    SettingsSave();
    delete ui;
    delete myport;
    delete ChartWidget;
}

/*定期实现相关信息的更新*/
void UpperComputer_HK::reflashinfo(){
    if(this->isHidden()){
        delete ChartWidget;
    }
    /*刷新串口box*/
    old_Comnum = now_Comnum;
    this->portinfo.clear();
    this->portinfo = QSerialPortInfo::availablePorts();
    now_Comnum = this->portinfo.count();
    if(now_Comnum != old_Comnum){
        ui->Cb_ComName->clear();
        for(int i=0;i<now_Comnum;i++){
            ui->Cb_ComName->addItem(portinfo.at(i).portName());
        }
    }
    if(!myport->isOpen()&&ui->Btn_OpenCom->text() == "打开串口"){
       myport->close();
       ui->Btn_OpenCom->setText("打开串口");
       ui->Cb_Baudrate->setEnabled(true);
       ui->Cb_ComName->setEnabled(true);
       ui->Cb_DataPos->setEnabled(true);
       ui->Cb_StopPos->setEnabled(true);
       ui->Cb_CheckPos->setEnabled(true);
    }
    this->error = myport->error();
    if(error == QSerialPort::ResourceError){
        myport->close();
        QMessageBox::question(this,tr("警告"),tr("串口已断开!"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
        ui->Btn_OpenCom->setText("打开串口");
        ui->Cb_Baudrate->setEnabled(true);
        ui->Cb_ComName->setEnabled(true);
        ui->Cb_DataPos->setEnabled(true);
        ui->Cb_StopPos->setEnabled(true);
        ui->Cb_CheckPos->setEnabled(true);
        return;
    }

    /*接收区刷新*/
    if(SendMode){
        this->curr_buff = ui->SendArea->toPlainText();
        this->pid_set = curr_buff.toDouble();
        this->curr_buff.clear();
    }else{
        this->byteCurr_Buff = myport->readAll();
        if(!byteCurr_Buff.isEmpty()){
            if(ui->Ckb_ChRcvd->checkState() == Qt::Checked){
                RcvdBuff = byteCurr_Buff;
            }else{
                curr_buff = byteCurr_Buff.toHex().data();
                curr_buff = curr_buff.toUpper();
                for(int i=0;i<curr_buff.length();i+=2){
                    QString str = curr_buff.mid(i,2);
                    RcvdBuff += str;
                    RcvdBuff += ' ';
                }

            }
            ui->RcvdArea->appendPlainText(RcvdBuff);
            RcvdBuff.clear();
            curr_buff.clear();
            byteCurr_Buff.clear();
        }
    }

    /*如果手动关闭图表窗口，则通过此代码快来完成相关信息的更新*/
    if(ChartWidget->isHidden()){
       ChartWidget->close();
       ui->Btn_PIDMode->setText("PID模式");
       if(SendMode){
           SendMode = !SendMode;
       }
    }

    /*在PID模式下不允许自动发送*/
    if(SendMode&&pid_set!=0){
        ui->LE_AutoSendTime->setFocusPolicy(Qt::NoFocus);

        this->curr_buff = myport->readAll();
        if(!this->curr_buff.isEmpty()){
            int count = 0;
            this->curr_buff1.clear();

            /*对于接收到的一堆数据的处理并将其打包*/
            for(int i=0;i<curr_buff.size();i++){
                if(curr_buff.at(i)=='\n'){
                    count++;
                    i+=2;
                    if(count==2) break;
                }
                if(count==1){
                    this->curr_buff1.append(curr_buff.at(i));
                }
            }
            /*打包完数据后清除所有垃圾数据*/
            this->curr_buff.clear();

            /*如果数据的开头是0或者. 证明该数据也是垃圾数据，直接返回*/
            if(curr_buff1.at(0)=='0'||curr_buff1.at(0)=='.') return;


            y=curr_buff1.toDouble()/pid_set;

            /*需要的数据处理完后也变成了垃圾数据，将其清理*/
            curr_buff1.clear();

            curr_point.setX(x);curr_point.setY(y);
            myLineSerials->append(curr_point);
            if(x >= 1){myLineSerials->clear();x=0;}
            else x+=0.05;
        }
    }else {
        myLineSerials->clear();
        x=0;y=0;
    }

    if(ui->Ckb_AutoSend->checkState() == Qt::Checked){
        old_autotime = now_autotime;
        now_autotime = ui->LE_AutoSendTime->text().toInt();
        if(now_autotime!=old_autotime){
            timerforsend->start(now_autotime);
        }
    }
    return;
}

/*初始化串口参数并且打开串口*/
bool UpperComputer_HK::SerialportConfig(){
    if(!myport->isOpen()){
        Comname     =   ui->Cb_ComName->currentText();
        Combaudrate =   ui->Cb_Baudrate->currentText();
        ComStopPos  =   ui->Cb_StopPos->currentText();
        ComDataPos  =   ui->Cb_DataPos->currentText();
        ComCheckPos =   ui->Cb_CheckPos->currentText();
        if(Comname.isEmpty()){
            return false;
        }else{
            /*获取比特率吧并且设置比特率*/
            int Baudrate = Combaudrate.toInt();
            myport->setBaudRate(Baudrate);
            /*设置串口号*/
            myport->setPortName(Comname);
            /*设置停止位*/
            if(ComStopPos == "1")           myport->setStopBits(QSerialPort::OneStop);
            else if(ComStopPos == "1.5")    myport->setStopBits(QSerialPort::OneAndHalfStop);
            else if(ComStopPos == "2")      myport->setStopBits(QSerialPort::TwoStop);
            /*设置数据位*/
            if(ComDataPos == "5")           myport->setDataBits(QSerialPort::Data5);
            else if(ComDataPos == "6")      myport->setDataBits(QSerialPort::Data6);
            else if(ComDataPos == "7")      myport->setDataBits(QSerialPort::Data7);
            else if(ComDataPos == "8")      myport->setDataBits(QSerialPort::Data8);
            /*设置校验位*/
            if(ComCheckPos == "None")       myport->setParity(QSerialPort::NoParity);
            else if(ComCheckPos == "Odd")   myport->setParity(QSerialPort::OddParity);
            else if(ComCheckPos == "Even")  myport->setParity(QSerialPort::EvenParity);
        }


        return myport->open(QSerialPort::ReadWrite);


    }else{

        myport->close();

        return true;
    }

    return false;
}

/*串口打开按钮的槽函数*/
void UpperComputer_HK::on_Btn_OpenCom_clicked()
{
    if(!SerialportConfig()){

        if(ui->Cb_ComName->currentText().isNull())
            QMessageBox::question(this,tr("警告"),tr("没有扫描到串口号!"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
        else
            QMessageBox::question(this,tr("警告"),tr("您的配置错误或者串口已经被占用!"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
        return;
    }

    if(myport->isOpen()){
        ui->Btn_OpenCom->setText("关闭串口");
        ui->Cb_Baudrate->setEnabled(false);
        ui->Cb_ComName->setEnabled(false);
        ui->Cb_DataPos->setEnabled(false);
        ui->Cb_StopPos->setEnabled(false);
        ui->Cb_CheckPos->setEnabled(false);
    }else{
        if(!ChartWidget->isHidden()){
            ChartWidget->close();
            ui->Btn_PIDMode->setText("PID模式");
        }
        ui->Btn_OpenCom->setText("打开串口");
        ui->Cb_Baudrate->setEnabled(true);
        ui->Cb_ComName->setEnabled(true);
        ui->Cb_DataPos->setEnabled(true);
        ui->Cb_StopPos->setEnabled(true);
        ui->Cb_CheckPos->setEnabled(true);
    }
}

/*初始化ui界面*/
void UpperComputer_HK::UiConfig(){
    /*设置接收区文本只可读*/
    ui->RcvdArea->setFocusPolicy(Qt::NoFocus);

    /*设置ui界面的大小*/
    this->setFixedSize(500,370);

    /*初始化模式为正常发送接收模式*/
    SendMode = false;

    /*设置初始界面的checkbox*/
    ui->Ckb_ChSend->setCheckState(Qt::Checked);
    ui->Ckb_AutoSend->setCheckState(Qt::Unchecked);
    ui->Ckb_HexSend->setCheckState(Qt::Unchecked);
    ui->Ckb_HexDisplay->setCheckState(Qt::Unchecked);
    ui->Ckb_ChRcvd->setCheckState(Qt::Checked);
    ui->LE_AutoSendTime->setFocusPolicy(Qt::NoFocus);
    ui->LE_P->setFocusPolicy(Qt::NoFocus);
    ui->LE_I->setFocusPolicy(Qt::NoFocus);
    ui->LE_D->setFocusPolicy(Qt::NoFocus);

    ui->RcvdArea->setPlainText("HK的个人博客https://hk-wjy.github.io/");
}

/*清除接收区*/
void UpperComputer_HK::on_Btn_ClearRcvd_clicked()
{
    ui->RcvdArea->clear();
    return;
}

/*清除发送区*/
void UpperComputer_HK::on_Btn_ClearSend_clicked()
{
    ui->SendArea->clear();
    return;
}

/*模式切换*/
void UpperComputer_HK::on_Btn_PIDMode_clicked()
{
    if(!myport->isOpen()){
        QMessageBox::question(this,tr("警告"),tr("没有打开串口!"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
        return;
    }
    SendMode = !SendMode;

    if(SendMode){
        ui->Btn_PIDMode->setText("正常模式");
        ui->LE_P->setFocusPolicy(Qt::ClickFocus);
        ui->LE_I->setFocusPolicy(Qt::ClickFocus);
        ui->LE_D->setFocusPolicy(Qt::ClickFocus);
        ui->RcvdArea->clear();
        ui->RcvdArea->appendPlainText("在PID模式下需要注意, 发送区域将用来发送目标值\n并且类型只能是数字,使用时注意删除该区域的文字,\n注意将所有参数与目标值设置好然后一起发送.");
        ChartWidget->show();

    }else{
        ui->Btn_PIDMode->setText("PID模式");
        ui->LE_P->setFocusPolicy(Qt::NoFocus);
        ui->LE_I->setFocusPolicy(Qt::NoFocus);
        ui->LE_D->setFocusPolicy(Qt::NoFocus);
        ChartWidget->close();
        ui->SendArea->clear();
    }
    return;
}

/*关于checkboxs的槽函数*/
void UpperComputer_HK::on_Ckb_HexDisplay_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked){
        ui->Ckb_ChRcvd->setCheckState(Qt::Checked);
    }else{
        ui->Ckb_ChRcvd->setCheckState(Qt::Unchecked);
    }
}
void UpperComputer_HK::on_Ckb_HexSend_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked){
        ui->Ckb_ChSend->setCheckState(Qt::Checked);
    }else{
        ui->Ckb_ChSend->setCheckState(Qt::Unchecked);
    }
}
void UpperComputer_HK::on_Ckb_ChRcvd_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked){
        ui->Ckb_HexDisplay->setCheckState(Qt::Checked);
    }else{
        ui->Ckb_HexDisplay->setCheckState(Qt::Unchecked);
    }
}
void UpperComputer_HK::on_Ckb_ChSend_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked){
        ui->Ckb_HexSend->setCheckState(Qt::Checked);
    }else{
        ui->Ckb_HexSend->setCheckState(Qt::Unchecked);
    }
}
void UpperComputer_HK::on_Ckb_AutoSend_stateChanged(int arg1)
{
    if(arg1 == Qt::Unchecked){
        timerforsend->stop();
        disconnect(timerforsend,&QTimer::timeout,this,&UpperComputer_HK::AutoSend);
        now_autotime = 0;
        old_autotime = now_autotime;
        ui->LE_AutoSendTime->setFocusPolicy(Qt::NoFocus);
    }else{
        old_autotime = now_autotime;
        now_autotime = ui->LE_AutoSendTime->text().toInt();
        timerforsend->start(now_autotime);
        connect(timerforsend,&QTimer::timeout,this,&UpperComputer_HK::AutoSend);
        ui->LE_AutoSendTime->setFocusPolicy(Qt::ClickFocus);
    }
}

/*发送按钮*/
void UpperComputer_HK::on_Btn_Send_clicked()
{
    if(SendMode){/*PID模式*/
        QByteArray buff;
        QByteArray buff1 = ui->SendArea->toPlainText().toUtf8();
        QByteArray buff2 = ui->LE_P->text().toUtf8();
        QByteArray buff3 = ui->LE_I->text().toUtf8();
        QByteArray buff4 = ui->LE_D->text().toUtf8();

        buff.append(buff1.size());        /*Data的信息位*/
        buff.append(buff2.size());        /*P参数的信息位*/
        buff.append(buff3.size());        /*I参数的信息位*/
        buff.append(buff4.size());        /*D参数的信息位*/

        for(int i=0;i<buff1.size();i++){
            if((buff1.at(i)>='0'&&buff1.at(i)<='9')||buff1.at(i)=='.'){buff.append(buff1.at(i));}
            else
            {
                QMessageBox::question(this,tr("警告"),tr("您输入的值不正确!\n(只允许输入数字!)"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
                ui->SendArea->clear();
                break;
                return;
            }
        }

        for(int i=0;i<buff2.size();i++){
            if((buff2.at(i)>='0'&&buff2.at(i)<='9')||buff2.at(i)=='.'){buff.append(buff2.at(i));}
            else
            {
                QMessageBox::question(this,tr("警告"),tr("您输入的值不正确!\n(只允许输入数字!)"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
                ui->LE_P->clear();
                break;
                return;
            }
        }

        for(int i=0;i<buff3.size();i++){
            if((buff3.at(i)>='0'&&buff3.at(i)<='9')||buff3.at(i)=='.'){buff.append(buff3.at(i));}
            else
            {
                QMessageBox::question(this,tr("警告"),tr("您输入的值不正确!\n(只允许输入数字!)"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
                ui->LE_I->clear();
                break;
                return;
            }
        }

        for(int i=0;i<buff4.size();i++){
            if((buff4.at(i)>='0'&&buff4.at(i)<='9')||buff4.at(i)=='.'){buff.append(buff4.at(i));}
            else
            {
                QMessageBox::question(this,tr("警告"),tr("您输入的值不正确!\n(只允许输入数字!)"),QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);
                ui->LE_D->clear();
                break;
                return;
            }
        }
        buff.append("#");
        myport->write(buff);
    }else{/*正常模式*/
        Send();
        return;
    }

}

/*图表初始化*/
void UpperComputer_HK::Chart_Config(){
    x=0,y=0;
    ChartWidget = new QChartView();
    PID_Chart = new QChart();
    myLineSerials = new QSplineSeries();

    mAxX = new QValueAxis();
    mAxY = new QValueAxis();
    mAxX->setRange(0,1);
    mAxY->setRange(0,2);
    mAxX->setTickCount(11);

    PID_Chart->addSeries(myLineSerials);
    PID_Chart->setTheme(QChart::ChartThemeBrownSand);

    mAxX->setTitleText("时间(单位/s)");
    mAxY->setTitleText("目前值与目标值的比值");

    PID_Chart->addAxis(mAxX,Qt::AlignBottom);
    PID_Chart->addAxis(mAxY,Qt::AlignLeft);

    myLineSerials->attachAxis(mAxX);
    myLineSerials->attachAxis(mAxY);

    PID_Chart->setBackgroundVisible(false);
    /*设置外边界全部为0*/
    PID_Chart->setContentsMargins(0, 0, 0, 0);
    /*设置内边界全部为0*/
    PID_Chart->setMargins(QMargins(0, 0, 0, 0));
    /*设置背景区域无圆角*/
    PID_Chart->setBackgroundRoundness(0);

    /*突出曲线上的点*/
    myLineSerials->setPointsVisible(true);

    /*图例*/
    QLegend *mlegend = PID_Chart->legend();
    myLineSerials->setName("PID_Chart");
    myLineSerials->setColor(QColor(255,0,0));

    /*在底部显示*/
    mlegend->setAlignment(Qt::AlignBottom);
    mlegend->show();

    /*将图表绑定到QChartView*/
    ChartWidget->setChart(PID_Chart);
}

/*配置保存函数，用来保存最后一次退出程序时的配置*/
void UpperComputer_HK::SettingsSave(){
    Configini = new QSettings("param.ini", QSettings::IniFormat);
    Configini->setValue("uartParam/BaudRate",ui->Cb_Baudrate->currentText());
    Configini->setValue("uartParam/DataBit",ui->Cb_DataPos->currentText());
    Configini->setValue("uartParam/Parity",ui->Cb_CheckPos->currentText());
    Configini->setValue("uartParam/StopBit",ui->Cb_StopPos->currentText());
    Configini->setValue("uartParam/P",ui->LE_P->text());
    Configini->setValue("uartParam/I",ui->LE_I->text());
    Configini->setValue("uartParam/D",ui->LE_D->text());
    delete Configini;
    return;
}
/*配置读取函数，读取之前保存的配置*/
void UpperComputer_HK::SettingsConfig(){
    Configini = new QSettings("param.ini", QSettings::IniFormat);
    Combaudrate = Configini->value("uartParam/BaudRate").toString();
    ComStopPos  = Configini->value("uartParam/StopBit").toString();
    ComDataPos  = Configini->value("uartParam/DataBit").toString();
    ComCheckPos = Configini->value("uartParam/Parity").toString();
    p = Configini->value("uartParam/P").toString();
    i = Configini->value("uartParam/I").toString();
    d = Configini->value("uartParam/D").toString();
    ui->Cb_CheckPos->setCurrentText(ComCheckPos);
    ui->Cb_Baudrate->setCurrentText(Combaudrate);
    ui->Cb_DataPos->setCurrentText(ComDataPos);
    ui->Cb_StopPos->setCurrentText(ComStopPos);
    ui->LE_P->setText(p);
    ui->LE_I->setText(i);
    ui->LE_D->setText(d);
    delete Configini;
    return;
}
void UpperComputer_HK::Send(){
    QByteArray buff;
    SendBuff = ui->SendArea->toPlainText();
    if(ui->Ckb_HexSend->checkState()==Qt::Checked){
        buff = QByteArray::fromHex(SendBuff.toLatin1().data());
    }else{
        buff = SendBuff.toUtf8();
    }
    myport->write(buff);
    SendBuff.clear();
}
void UpperComputer_HK::AutoSend(){
    Send();
}
