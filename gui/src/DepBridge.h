#pragma once
#include <QString>
#include <QStringList>
#include <vector>
#include "DepParser.h"

struct SeriesPoint { int step; double bu; double days; double value; };

class DepBridge {
public:
    bool loadFile(const QString& path, QString& error);
    bool hasData() const { return has_; }
    
    QStringList materials() const;
    QStringList parameters(const QString& material) const;
    QStringList nuclides() const; // "Total" + ізотопи без Lost/Total

    std::vector<SeriesPoint> series(const QString& material,
                                    const QString& param,
                                    const QString& nuclideOrTotal) const;

private:
    int calcNiso_() const;
    bool has_ = false;
    DepData data_;
};
