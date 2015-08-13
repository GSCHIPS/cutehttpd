#include "client.h"
#include "requestresponsehelper.h"

#include <QDir>
#include <QFile>
#include <QNetworkRequest>
#include <QUrl>
#include <QHostAddress>

Client::Client(qintptr descr, QString absolutePath, QObject *parent) : QObject(parent)
{

    socket = new QTcpSocket(this);
    connect(socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()));

    socket->setSocketDescriptor(descr);
    m_rootFolder = absolutePath;
}

void Client::readyRead() {
    //Get url
    QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));
    QString url = tokens[1];
    QByteArray response;

    //Url switch
    if (url == "/style.css") {
        //Send page
        QFile page(":/frontend/frontend/style.css");
        if(!page.open(QIODevice::ReadOnly)) {
            qDebug() << "Unable to open file";
        }
        response.append(RequestResponseHelper::createHeader(page, RequestResponseHelper::MIME_TYPE_TEXT_CSS));
        response.append(page.readAll());
        page.close();
    }
    if (url == "/") {
        //Send page
        QFile page(":/frontend/frontend/index.html");
        if(!page.open(QIODevice::ReadOnly)) {
            qDebug() << "Unable to open file";
        }
        response.append(RequestResponseHelper::createHeader(page, RequestResponseHelper::MIME_TYPE_TEXT_HTML));
        response.append(page.readAll());
        page.close();
    }
    if (url == "/folder.png") {
        //Send image
        QFile image(":/frontend/frontend/folder.png");
        if(!image.open(QIODevice::ReadOnly)) {
            qDebug() << "Unable to open file";
        }
        response.append(RequestResponseHelper::createHeader(image, RequestResponseHelper::MIME_TYPE_IMAGE_PNG));
        response.append(image.readAll());
        image.close();
    }
    if (url == "/file.png") {
        //Send image
        QFile image(":/frontend/frontend/file.png");
        if(!image.open(QIODevice::ReadOnly)) {
            qDebug() << "Unable to open file";
        }
        response.append(RequestResponseHelper::createHeader(image, RequestResponseHelper::MIME_TYPE_IMAGE_PNG));
        response.append(image.readAll());
        image.close();
    }
    if (url == "/cd") {
        //Change directory request
        QString request = socket->readAll();
//        qDebug() << request;
        QMap<QString, QString> *params = RequestResponseHelper::getPostParams(request);
        QString path = params->value("path", "");
        if (path.isEmpty()) {
            //Error
            qDebug() << "[Error] Wrong request";
            delete params;
            socket->close();
            return;
        }
        QDir dir(m_rootFolder + path);
        qDebug() << "Client requested folder : " << dir.absolutePath();
        if (!dir.absolutePath().startsWith(m_rootFolder)) {
            qDebug() << "[Error] Folder " + dir.path() + " is out of shared folder";
            delete params;
            socket->close();
            return;
        }
        if (!dir.exists()) {
            //Error
            qDebug() << "[Error] Folder " + path + " not exist";
            delete params;
            socket->close();
            return;
        }

        //Get all folders
        dir.setFilter(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        QStringList folders = dir.entryList();

        //Get all files
        dir.setFilter(QDir::Files | QDir::NoSymLinks);
        QStringList files = dir.entryList();

        response.append(RequestResponseHelper::composeJsonResponse(path, folders, files));
        delete params;
    }
    if (url.startsWith("/download")) {
        //Download file request
        QString filePath = QUrl::fromPercentEncoding(url.split("=").last().toUtf8());
        QFile file(m_rootFolder + filePath);
        if (!file.fileName().startsWith(m_rootFolder)) {
            qDebug() << "[Error] File " + file.fileName() + " is out of shared folder";
            socket->close();
            return;
        }
        if (!file.exists()) {
            qDebug() << "File not found";
            socket->close();
            socket->deleteLater();
            return;
        }
        qDebug() << "Client downloading file " << file.fileName();
        file.open(QIODevice::ReadOnly);
        socket->write(RequestResponseHelper::createHeader(file, RequestResponseHelper::MIME_TYPE_APPLICATION_OCTET_STREAM).toUtf8());
        socket->waitForBytesWritten();
        qint64 blockSize = 1000;
        file.seek(0);
        while (!file.atEnd()) {
            socket->write(file.read(blockSize));
            socket->waitForBytesWritten();
        }
        file.close();
        socket->close();
        socket->deleteLater();
        return;
    }
    socket->write(response);
    socket->waitForBytesWritten();
    socket->close();
    socket->deleteLater();
}

void Client::connected() {
}

void Client::disconnected() {
    this->deleteLater();
}