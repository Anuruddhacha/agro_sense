#pragma once

#include <QFrame>

class QLabel;

class SensorCard : public QFrame
{
    Q_OBJECT

public:
    explicit SensorCard(const QString &title,
                        const QString &icon,
                        const QString &unit,
                        QWidget *parent = nullptr);

    void setValue(const QString &value);
    void setSubValue(const QString &subValue);
    void setStatus(const QString &status, const QString &level = QStringLiteral("ok"));

private:
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_valueLabel = nullptr;
    QLabel *m_unitLabel = nullptr;
    QLabel *m_subLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
};
