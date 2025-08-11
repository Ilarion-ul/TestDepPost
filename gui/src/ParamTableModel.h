#pragma once
#include <QAbstractTableModel>
#include "DepBridge.h"

class ParamTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit ParamTableModel(QObject* parent=nullptr);
    void setSeries(std::vector<SeriesPoint> s);

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex&, int role) const override;
    QVariant headerData(int, Qt::Orientation, int role) const override;

private:
    std::vector<SeriesPoint> data_;
};
