#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include <QTime>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _recv = new RecvThread();
    _send = new SendThread();
    _recv->start();
    _send->start();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString srcip = ui->lineEdit_3->text();

    QString dstip = ui->lineEdit->text();
    QString path = ui->lineEdit_2->text();
    //_send->onSendFile(srcip,dstip,path);

    emit _send->signaleSendFile(srcip,dstip,path);
    qDebug()<<"sendFile time "<<QTime::currentTime();
}

void MainWindow::on_pushButton_2_clicked()
{
   QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), "", tr("Image Files (*.*)"));
   ui->lineEdit_2->setText(fileName);
}
