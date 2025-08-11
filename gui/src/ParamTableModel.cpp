#include "ParamTableModel.h"

ParamTableModel::ParamTableModel(QObject* p):QAbstractTableModel(p){}

void ParamTableModel::setSeries(std::vector<SeriesPoint> s){
    beginResetModel(); data_=std::move(s); endResetModel();
}
int ParamTableModel::rowCount(const QModelIndex&) const { return (int)data_.size(); }
int ParamTableModel::columnCount(const QModelIndex&) const { return 4; } // Step, BU, DAYS, Value

QVariant ParamTableModel::data(const QModelIndex& i, int role) const {
    if (!i.isValid() || role!=Qt::DisplayRole) return {};
    const auto& r = data_[i.row()];
    switch (i.column()) { case 0: return r.step; case 1: return r.bu; case 2: return r.days; case 3: return r.value; }
    return {};
}
QVariant ParamTableModel::headerData(int s, Qt::Orientation o, int role) const {
    if (role!=Qt::DisplayRole) return {};
    if (o==Qt::Horizontal) { static const char* H[]={"Step","BU","DAYS","Value"}; return H[s]; }
    return s+1;
}
