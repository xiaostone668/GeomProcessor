#pragma once
#ifndef GEOM_RECEIVER_H
#define GEOM_RECEIVER_H

#include <QObject>
#include <QSharedMemory>
#include <QTimer>
#include <QString>
#include "GeomIPC.h"

/**
 * @brief 接收来自 SimulationTool 的几何数据
 *
 * 轮询共享内存（50 ms），检测 CMD_SEND_GEOM 命令，
 * 发出 geometryReceived(filePath) 信号。
 */
class GeomReceiver : public QObject
{
    Q_OBJECT
public:
    explicit GeomReceiver(QObject* parent = nullptr);
    ~GeomReceiver() override;

    bool start();
    void stop();
    bool isRunning() const { return m_running; }

    /** 向 SimulationTool 发送结果通知 */
    bool sendResult(const QString& resultFilePath);
    /** 向 SimulationTool 发送错误通知 */
    bool sendError(const QString& errorMsg);

signals:
    void geometryReceived(const QString& stepFilePath, uint32_t seqNo);
    void statusChanged(const QString& status);
    void errorOccurred(const QString& error);

private slots:
    void poll();

private:
    bool readBlock(GeomIPCBlock& blk);
    bool writeBlock(const GeomIPCBlock& blk);
    bool attach();

    QSharedMemory* m_shm    = nullptr;
    QTimer*        m_timer  = nullptr;
    bool           m_running= false;
    uint32_t       m_lastSeq= 0;
};

#endif // GEOM_RECEIVER_H
