#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <QObject>
#include <QThread>

class Controller : public QObject
{
    Q_OBJECT
public:
    Controller(qintptr descr, QString &absolutePath);
};

#endif // CONTROLLER_H
