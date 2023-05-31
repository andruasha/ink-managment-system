#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <QString>
#include <QWidget>
#include <QLabel>
#include <QVector>
#include "mraa.hpp"

#define NUM_PINS 8 //Число цветовых каналов.
bool globalFlag = true; //Глобальный флаг, сигнализирующий о ЧС.


//Класс считывания и обработки данных с субтанков.
MyThreadClass::MyThreadClass(QObject *parent, QWidget *pumWidget, QWidget *subWidget, mraa::Gpio *pumPin, mraa::Gpio *subPin) : QObject(parent) {
    this->subWidget = subWidget;
    this->pumWidget = pumWidget;
    this->pumPin = pumPin;
    this->subPin = subPin;
}

//Исполняемая функция
void MyThreadClass::process() {
    bool localFlag = true; //Локальный флаг, сигнализирующий о ЧС в текущем потоке.
    time_t timeStart = 0; //Время начала работы насоса.

    // Цикл считывания и обработки данных с субтанка.
    while (localFlag) {
        time_t timeNow = time(nullptr);
        int value = subPin->read();

        /// Обработка возникновения ЧС в другом потоке.
        if (!globalFlag) {
            pumPin->write(0);
            setColor(pumWidget, subWidget, -1);
            localFlag = false;
        }

        //Обработка события, когда в субтанке при выключенном насосе кончились чернила.
        else if (value == 0 && timeStart == 0) {
            pumPin->write(1);
            setColor(pumWidget, subWidget, 0);
            timeStart = time(nullptr);
        }

        //Обработка события, когда субтанк наполнился чернилами.
        else if (difftime(timeNow, timeStart) >= 2 && value == 1) {
            pumPin->write(0);
            timeStart = 0;
            setColor(pumWidget, subWidget, 1);
        }

        //Обработка ЧС, когда субтак не наполнился за 60 секунд.
        else if(difftime(timeNow, timeStart) >= 60 && value == 0) {
            pumPin->write(0);
            timeStart = 0;
            setColor(pumWidget, subWidget, -1);
            globalFlag = false;
            QThread::sleep(1);
            setLabel(pumPin, subPin);
        }

        //Приостанавливаем работу потока на 1 секунду для разгрузки процессора.
        QThread::sleep(1);
    }
}

MyThreadClass::~MyThreadClass() {
    delete pumPin;
    delete subPin;
}



//Класс визуализации.
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    //Инициализируем графический интерфейс.
    ui->setupUi(this);

    //Инициализируем виджеты индикации состояния помп.
    pumWidget[0] = ui->c_pum;
    pumWidget[1] = ui->m_pum;
    pumWidget[2] = ui->y_pum;
    pumWidget[3] = ui->k_pum;
    pumWidget[4] = ui->lc_pum;
    pumWidget[5] = ui->lm_pum;
    pumWidget[6] = ui->w_pum;
    pumWidget[7] = ui->v_pum;

    //Инициализируем виджеты индикации состояния субтанков.
    subWidget[0] = ui->c_sub;
    subWidget[1] = ui->m_sub;
    subWidget[2] = ui->y_sub;
    subWidget[3] = ui->k_sub;
    subWidget[4] = ui->lc_sub;
    subWidget[5] = ui->lm_sub;
    subWidget[6] = ui->w_sub;
    subWidget[7] = ui->v_sub;


    //Инициализируем GPIO объекты помп.
    pumPin[0] = new mraa::Gpio(37);
    pumPin[1] = new mraa::Gpio(32);
    pumPin[2] = new mraa::Gpio(23);
    pumPin[3] = new mraa::Gpio(40);
    pumPin[4] = new mraa::Gpio(24);
    pumPin[5] = new mraa::Gpio(21);
    pumPin[6] = new mraa::Gpio(19);
    pumPin[7] = new mraa::Gpio(16);

    //Инициализируем GPIO объекты субтанков.
    subPin[0] = new mraa::Gpio(5);
    subPin[1] = new mraa::Gpio(7);
    subPin[2] = new mraa::Gpio(8);
    subPin[3] = new mraa::Gpio(3);
    subPin[4] = new mraa::Gpio(10);
    subPin[5] = new mraa::Gpio(11);
    subPin[6] = new mraa::Gpio(13);
    subPin[7] = new mraa::Gpio(22);

    

    //Выставляем работу пинов субтанков на вход, помп - на выход.
    for (int i=0; i<NUM_PINS; i++) {
        subPin[i]->dir(mraa::DIR_IN);
        pumPin[i]->dir(mraa::DIR_OUT);
    }


    QVector<QThread*> threads; //Вектор потоков.
    QVector<MyThreadClass*> threadObjects; //Вектор экземпляров класса MyThreadClass.


    //Распределение цветовых каналов по потокам.
    for (int i = 0; i < NUM_PINS; ++i) {
        QThread* thread = new QThread;
        MyThreadClass* object = new MyThreadClass(nullptr, pumWidget[i], subWidget[i], pumPin[i], subPin[i]);
        object->moveToThread(thread);
        QObject::connect(thread, &QThread::started, object, &MyThreadClass::process);
        connect(object, &MyThreadClass::setColor, this, &MainWindow::setColor);
        connect(object, &MyThreadClass::setLabel, this, &MainWindow::setLabel);
        threads.append(thread);
        threadObjects.append(object);
    }

    //Запуск потоков.
    for (int i = 0; i < threads.count(); ++i) {
        threads.at(i)->start();
    }
}

//Метод изменения цвета виджетов индикации помп и субтанков.
void MainWindow::setColor(QWidget *pumWidget, QWidget *subWidget, int value) {
    if (value == 1) {
        pumWidget->setStyleSheet("background-color: red; border-radius: 20px;");
        subWidget->setStyleSheet("background-color: green; border-radius: 20px;");
    }

    else if (value == 0) {
        pumWidget->setStyleSheet("background-color: green; border-radius: 20px;");
        subWidget->setStyleSheet("background-color: red; border-radius: 20px;");
    }

    else if (value == -1) {
        pumWidget->setStyleSheet("background-color: red; border-radius: 20px;");
        subWidget->setStyleSheet("background-color: red; border-radius: 20px;");
    }
}

//Метод вывода сообщения при возникновении ЧС.
void MainWindow::setLabel(mraa::Gpio *pumPin, mraa::Gpio *subPin) {
    ui->pum_label->hide();
    ui->sub_label->hide();

    ui->c_letter->hide();
    ui->m_letter->hide();
    ui->y_letter->hide();
    ui->k_letter->hide();
    ui->lc_letter->hide();
    ui->lm_letter->hide();
    ui->w_letter->hide();
    ui->v_letter->hide();

    ui->c_pum->hide();
    ui->m_pum->hide();
    ui->y_pum->hide();
    ui->k_pum->hide();
    ui->lc_pum->hide();
    ui->lm_pum->hide();
    ui->w_pum->hide();
    ui->v_pum->hide();

    ui->c_sub->hide();
    ui->m_sub->hide();
    ui->y_sub->hide();
    ui->k_sub->hide();
    ui->lc_sub->hide();
    ui->lm_sub->hide();
    ui->w_sub->hide();
    ui->v_sub->hide();

    ui->c_widget->hide();
    ui->m_widget->hide();
    ui->y_widget->hide();
    ui->k_widget->hide();
    ui->lc_widget->hide();
    ui->lm_widget->hide();
    ui->w_widget->hide();
    ui->v_widget->hide();

    QString pumpErrorText = "ERROR";
    QString subtErrorText = "Работа была прекращена\nНасос №" + QString::number(pumPin->getPin()) + " для субтанка №" + QString::number(subPin->getPin()) + " работал более 10 секунд\nПроверьте исправность насоса и субтанка\nПосле диагностики перезагрузите устройство";

    ui->pump_error->setText(pumpErrorText);
    ui->subt_error->setText(subtErrorText);
}

MainWindow::~MainWindow() {
    delete ui;
}

