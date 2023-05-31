#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QThread>
#include <QLabel>
#include "mraa.hpp"

#define NUM_PINS 8

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MyThreadClass : public QObject
{
    Q_OBJECT
public:
    explicit MyThreadClass(QObject *parent = nullptr, QWidget *pumWidget = 0, QWidget *subWidget = 0, mraa::Gpio *pumPin = 0, mraa::Gpio *subPin = 0);
    QWidget *pumWidget;
    QWidget *subWidget;
    mraa::Gpio *subPin;
    mraa::Gpio *pumPin;
    ~MyThreadClass();

public slots:
    void process();

signals:
    void setColor(QWidget *pumWidget, QWidget *subWidget, int value);
    void setLabel(mraa::Gpio *pumPin, mraa::Gpio *subPin);
};



class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QWidget *pumWidget[NUM_PINS];
    QWidget *subWidget[NUM_PINS];
    mraa::Gpio *subPin[NUM_PINS];
    mraa::Gpio *pumPin[NUM_PINS];

private slots:
    void setColor(QWidget *pumWidget, QWidget *subWidget, int value);
    void setLabel(mraa::Gpio *pumPin, mraa::Gpio *subPin);
};



#endif // MAINWINDOW_H
