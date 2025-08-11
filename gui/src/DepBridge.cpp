#include "DepBridge.h"
#include "Logger.h"
#include <algorithm>

static bool isLostOrTotalName(const std::string& s) {
    return s=="lost"||s=="Lost"||s=="LOST"||s=="total"||s=="Total"||s=="TOTAL";
}

int DepBridge::calcNiso_() const {
    if (!data_.ZAI.empty()) {
        int n = 0; for (int z : data_.ZAI) if (z!=0 && z!=666) ++n; return n;
    }
    int n = 0; for (auto& nm : data_.NAMES) if (!isLostOrTotalName(nm)) ++n; return n;
}

bool DepBridge::loadFile(const QString& path, QString& error) {
    try {
        DepParser p; p.parseFile(path.toStdString());
        data_ = p.getData();
        has_ = true;
        slog::Logger::info("Loaded dep.m. steps={}, isotopes={}",
                           data_.steps.size(), data_.ZAI.size());
        return true;
    } catch (const std::exception& ex) {
        error = ex.what();
        slog::Logger::error("Failed to load dep.m: {}", error.toStdString());
        has_ = false;
        return false;
    }
}

QStringList DepBridge::materials() const {
    if (!has_ || data_.steps.empty()) return {};
    const auto& mats = data_.steps.begin()->second;
    QStringList out; out.reserve((int)mats.size());
    for (auto& kv : mats) out << QString::fromStdString(kv.first);
    std::sort(out.begin(), out.end(), [](auto&a,auto&b){return a.toLower()<b.toLower();});
    return out;
}

QStringList DepBridge::parameters(const QString& material) const {
    if (!has_ || data_.steps.empty()) return {};
    const auto& step0 = data_.steps.begin()->second;
    auto it = step0.find(material.toStdString());
    if (it == step0.end()) return {};
    QStringList out; for (auto& kv : it->second) out << QString::fromStdString(kv.first);
    std::sort(out.begin(), out.end(), [](auto&a,auto&b){return a.toLower()<b.toLower();});
    return out;
}

QStringList DepBridge::nuclides() const {
    if (!has_) return {};
    QStringList out; out << "Total";
    const int niso = calcNiso_();
    if (!data_.NAMES.empty()) {
        for (int i=0;i<niso;++i) out << QString::fromStdString(data_.NAMES[i]);
    } else {
        for (int i=0;i<niso;++i) out << QString("I%1").arg(i+1);
    }
    return out;
}

std::vector<SeriesPoint> DepBridge::series(const QString& material,
                                           const QString& param,
                                           const QString& nuclideOrTotal) const {
    std::vector<SeriesPoint> out;
    if (!has_) return out;

    const bool wantTotal = (nuclideOrTotal.compare("Total", Qt::CaseInsensitive)==0);
    const int  niso = calcNiso_();

    auto totalIndexOf = [&](const std::vector<double>& v)->int {
        if ((int)v.size()==1) return 0;                 // Scalar → лише Total
        if ((int)v.size()==(niso+2)) return (int)v.size()-1; // ... Lost, Total
        if ((int)v.size()==(niso+1)) return (int)v.size()-1; // ... Total
        return (int)v.size()-1;
    };

    auto isoIndexOf = [&](const QString& name)->int {
        if (data_.NAMES.empty()) return -1;
        for (int i=0;i<niso;++i)
            if (QString::fromStdString(data_.NAMES[i]).compare(name, Qt::CaseInsensitive)==0) return i;
        return -1;
    };
    const int isoIdx = wantTotal ? -1 : isoIndexOf(nuclideOrTotal);

    std::vector<int> steps; steps.reserve(data_.steps.size());
    for (auto& kv : data_.steps) steps.push_back(kv.first);
    std::sort(steps.begin(), steps.end());

    for (int s : steps) {
        const auto& mats = data_.steps.at(s);
        auto mit = mats.find(material.toStdString());
        if (mit==mats.end()) continue;
        auto pit = mit->second.find(param.toStdString());
        if (pit==mit->second.end()) continue;

        const auto& vec = pit->second; if (vec.empty()) continue;

        double val=0.0;
        if (wantTotal) {
            const int ti = totalIndexOf(vec);
            if (ti<0 || ti>=(int)vec.size()) continue;
            val = vec[ti];
        } else {
            if (isoIdx<0 || isoIdx>=(int)vec.size()) continue;
            val = vec[isoIdx];
        }

        const int row = s; // step==рядок (за твоєю моделлю)
        const double bu   = (row<(int)data_.BU.size())   ? data_.BU[row]   : (double)row;
        const double days = (row<(int)data_.DAYS.size()) ? data_.DAYS[row] : (double)row;

        out.push_back({s, bu, days, val});
    }
    return out;
}
