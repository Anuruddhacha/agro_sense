#include "SensorCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

SensorCard::SensorCard(const QString &title, const QString &icon, const QString &unit, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("sensorCard"));
    setProperty("statusLevel", QStringLiteral("ok"));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 14, 16, 14);
    root->setSpacing(8);

    auto *header = new QHBoxLayout;
    m_iconLabel = new QLabel(icon);
    m_iconLabel->setObjectName(QStringLiteral("cardIcon"));
    m_titleLabel = new QLabel(title);
    m_titleLabel->setObjectName(QStringLiteral("cardTitle"));
    m_statusLabel = new QLabel(QStringLiteral("OK"));
    m_statusLabel->setObjectName(QStringLiteral("cardStatus"));
    m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    header->addWidget(m_iconLabel);
    header->addWidget(m_titleLabel, 1);
    header->addWidget(m_statusLabel);
    root->addLayout(header);

    auto *valueRow = new QHBoxLayout;
    m_valueLabel = new QLabel(QStringLiteral("--"));
    m_valueLabel->setObjectName(QStringLiteral("cardValue"));
    m_unitLabel = new QLabel(unit);
    m_unitLabel->setObjectName(QStringLiteral("cardUnit"));
    m_unitLabel->setAlignment(Qt::AlignBottom);

    valueRow->addWidget(m_valueLabel);
    valueRow->addWidget(m_unitLabel);
    valueRow->addStretch();
    root->addLayout(valueRow);

    m_subLabel = new QLabel;
    m_subLabel->setObjectName(QStringLiteral("cardSub"));
    root->addWidget(m_subLabel);
}

void SensorCard::setValue(const QString &value)
{
    m_valueLabel->setText(value);
}

void SensorCard::setSubValue(const QString &subValue)
{
    m_subLabel->setText(subValue);
    m_subLabel->setVisible(!subValue.isEmpty());
}

void SensorCard::setStatus(const QString &status, const QString &level)
{
    m_statusLabel->setText(status);
    setProperty("statusLevel", level);
    style()->unpolish(this);
    style()->polish(this);
}
