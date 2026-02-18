#include "GeomReceiver.h"
#include <QTimer>
#include <cstring>

GeomReceiver::GeomReceiver(QObject* parent)
    : QObject(parent)
{
    m_shm   = new QSharedMemory(GEOM_IPC_KEY, this);
    m_timer = new QTimer(this);
    m_timer->setInterval(100); // 100 ms poll
    connect(m_timer, &QTimer::timeout, this, &GeomReceiver::poll);
}

GeomReceiver::~GeomReceiver()
{
    stop();
}

bool GeomReceiver::start()
{
    if (m_running) return true;

    // Try attaching (SimulationTool may have created it)
    if (!attach()) {
        emit statusChanged("等待 SimulationTool 启动共享内存...");
    }

    m_timer->start();
    m_running = true;
    emit statusChanged("IPC 接收器已启动，等待几何数据...");
    return true;
}

void GeomReceiver::stop()
{
    if (!m_running) return;
    m_timer->stop();
    if (m_shm->isAttached()) m_shm->detach();
    m_running = false;
}

bool GeomReceiver::attach()
{
    if (m_shm->isAttached()) return true;

    // First try to attach (if SimulationTool created it)
    if (m_shm->attach(QSharedMemory::ReadWrite)) {
        // Reset any leftover command so GeomProcessor always starts empty
        m_shm->lock();
        GeomIPCBlock blk;
        memcpy(&blk, m_shm->constData(), sizeof(blk));
        blk.cmd = CMD_IDLE;
        memcpy(m_shm->data(), &blk, sizeof(blk));
        m_shm->unlock();
        return true;
    }

    // If not available, create our own (so SimulationTool can attach later)
    if (m_shm->create(GEOM_IPC_SIZE, QSharedMemory::ReadWrite)) {
        // Zero-init
        m_shm->lock();
        GeomIPCBlock blk;
        memcpy(m_shm->data(), &blk, sizeof(blk));
        m_shm->unlock();
        return true;
    }

    return false;
}

void GeomReceiver::poll()
{
    if (!m_shm->isAttached()) {
        if (!attach()) return;
    }

    GeomIPCBlock blk;
    if (!readBlock(blk)) return;

    if (blk.magic != (uint32_t)GEOM_IPC_MAGIC) return;

    if (blk.cmd == CMD_SEND_GEOM && blk.seqNo != m_lastSeq) {
        m_lastSeq = blk.seqNo;
        QString path = QString::fromLocal8Bit(blk.geomFilePath);
        emit statusChanged(QString("收到几何数据 #%1: %2").arg(blk.seqNo).arg(path));
        emit geometryReceived(path, blk.seqNo);

        // Acknowledge: set CMD_PROCESSING
        blk.cmd = CMD_PROCESSING;
        writeBlock(blk);
    }
}

bool GeomReceiver::sendResult(const QString& resultFilePath)
{
    if (!m_shm->isAttached() && !attach()) return false;

    GeomIPCBlock blk;
    readBlock(blk);
    blk.cmd = CMD_RESULT_READY;
    QByteArray ba = resultFilePath.toLocal8Bit();
    strncpy(blk.resultFilePath, ba.constData(), sizeof(blk.resultFilePath) - 1);
    return writeBlock(blk);
}

bool GeomReceiver::sendError(const QString& errorMsg)
{
    if (!m_shm->isAttached() && !attach()) return false;

    GeomIPCBlock blk;
    readBlock(blk);
    blk.cmd = CMD_ERROR;
    QByteArray ba = errorMsg.toLocal8Bit();
    strncpy(blk.errorMsg, ba.constData(), sizeof(blk.errorMsg) - 1);
    return writeBlock(blk);
}

bool GeomReceiver::readBlock(GeomIPCBlock& blk)
{
    if (!m_shm->lock()) return false;
    memcpy(&blk, m_shm->constData(), sizeof(GeomIPCBlock));
    m_shm->unlock();
    return true;
}

bool GeomReceiver::writeBlock(const GeomIPCBlock& blk)
{
    if (!m_shm->lock()) return false;
    memcpy(m_shm->data(), &blk, sizeof(GeomIPCBlock));
    m_shm->unlock();
    return true;
}
