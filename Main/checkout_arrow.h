#ifndef CHECKOUTARROW_H
#define CHECKOUTARROW_H

#undef max
#undef min
#undef signals

#include <arrow/api.h>
#include <arrow/filesystem/filesystem.h>
#include <arrow/util/iterator.h>
#include <arrow/ipc/writer.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/api.h>

#include <QString>
#include <QMap>
#include <QSet>
#include <QDebug>
#include <QDir>


namespace fs = arrow::fs;

template<class K,class V>
struct QMapWrapper {
    const QMap<K,V> map;
    QMapWrapper(const QMap<K,V>& map) : map(map) {}
    typename QMap<K,V>::const_key_value_iterator begin() { return map.keyValueBegin(); }
    typename QMap<K,V>::const_key_value_iterator end()   { return map.keyValueEnd();   }
};
template<class K,class V>
QMapWrapper<K,V> wrapQMap(const QMap<K,V>& map) {
    return QMapWrapper<K,V>(map);
}

#define ArrowGet(var, id, call, msg) \
    auto id = call; if (!id.ok()) { qDebug() << msg << QString::fromStdString(id.status().message()); return ; } auto var = id.ValueOrDie();

#define ArrowBreak(var, id, call, msg) \
    auto id = call; if (!id.ok()) { qDebug() << msg << QString::fromStdString(id.status().message()); break ; } auto var = id.ValueOrDie();

template <class T>
struct mytrait
{
    typedef typename T::value_type   value_type;
};

template<>
struct mytrait<class arrow::StringArray>
{
    typedef std::string value_type;
};

template <class T>
inline typename mytrait<T>::value_type  _get(std::shared_ptr<T> ar, int s)
{
    return ar->Value(s);
}


template<>
inline mytrait<arrow::StringArray>::value_type _get(std::shared_ptr<arrow::StringArray> ar, int s)
{
    return ar->GetString(s);
}


template <class Bldr, class ColType>
inline void concat(const QList<std::shared_ptr<arrow::Array> >& list, std::shared_ptr<arrow::Array>& res, QList<QList<bool> > de_doubler)
{
    Bldr bldr;

    int d = 0;
    for (auto& ar: list)
    {
        auto c = std::static_pointer_cast<ColType>(ar);
        for (int s = 0; s < c->length(); ++s)
        {
            if ( s < de_doubler[d].size() &&  de_doubler[d][s])
            {
                if (c->IsValid(s))
                {
                    auto status = bldr.Append(_get(c, s));
                    //   qDebug() << s; // << QString("%1").arg(_get(c, s));
                }
                else
                {
                    //   qDebug() << s << "-";
                    auto status = bldr.AppendNull();
                }
            }
            else
            {
                if (s < de_doubler[d].size())
                    qDebug() << "Skipped Value" << d << s;
            }
        }

        d++;
    }

    auto status = bldr.Finish(&res);

}


inline double AggregateSum(QList<float>& f)
{

    if (f.size() == 1) return f.at(0);

    double r = 0;
    foreach(double ff, f)
        r += ff;

    return r;
}

inline double AggregateMean(QList<float>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = 0;
    foreach(double ff, f)
        r += ff;
    r /= (double)f.size();
    return r;
}




inline double AggregateMedian(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    std::sort(f.begin(), f.end());
    return f.at(f.size() / 2);
}

inline double AggregateMin(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::max();
    foreach(double ff, f)
        if (ff < r) r = ff;
    return r;
}

inline double AggregateMax(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::min();
    foreach(double ff, f)
        if (ff > r) r = ff;
    return r;
}


inline float Aggregate(QList<float>& f, QString& ag)
{
    if (ag == "Sum") return AggregateSum(f);
    if (ag == "Mean") return AggregateMean(f);
    if (ag == "Median") return AggregateMedian(f);
    if (ag == "Min") return AggregateMin(f);
    if (ag == "Max") return AggregateMax(f);

    qDebug() << "Aggregation error!" << ag << "Not found";
    return -1.;
}


inline QList<bool> double_checker(QSet<QString>& skiper, std::shared_ptr<arrow::RecordBatch> rb)
{
    QList<bool> res;
    auto well = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("Well"));
    auto field = std::static_pointer_cast<arrow::Int16Array>(rb->GetColumnByName("fieldId"));
    auto slice = std::static_pointer_cast<arrow::Int16Array>(rb->GetColumnByName("sliceId"));
    auto timep = std::static_pointer_cast<arrow::Int16Array>(rb->GetColumnByName("timepoint"));
    auto channel = std::static_pointer_cast<arrow::Int16Array>(rb->GetColumnByName("channel"));


    for (int i = 0; i < well->length(); ++i)
    {
        auto wl = QString("%1%2%3%4%5").arg(QString::fromStdString(well->GetString(i)))
                .arg(field->Value(i)).arg(slice->Value(i)).arg(timep->Value(i)).arg(channel->Value(i));
        res.append(!skiper.contains(wl));
        skiper.insert(wl);

    }

    return res;
}



template <class T>
bool is_arrow_type(QList<  std::shared_ptr<arrow::Array> >& list)
{

    bool res = false;

    for (auto c: list) if (std::dynamic_pointer_cast<T>(c))  res= true;

    return res;
}


inline void fuseArrow(QString bp, QStringList files, QString out, QString plateID)
{
    qDebug() << "Fusing" << files << "to" << out;

    QMap<std::string, QList<  std::shared_ptr<arrow::Array> > > datas;
    QMap<std::string, std::shared_ptr<arrow::Field> >     fields;
    QMap<std::string, QMap<std::string, QList<float> > >  agg;
    QMap<std::string, std::string> tgs;

    QList<QList<bool> > de_doubler; // to remove the double
    QSet<QString> skiper; // to track the doubles

    QList<int> per_file_counter;

    int counts = 0;

    for (auto file : files)
    {
        QList<std::string> lists;
        QString ur = QString(bp + "/" + file).replace("\\", "/").replace("//", "/");

        std::string uri = ur.toStdString(), root_path;
        ArrowBreak(fs, r0, fs::FileSystemFromUriOrPath(uri, &root_path), "Arrow File not loading" << file << ur << QString::fromStdString(root_path));

        ArrowBreak(input, r1, fs->OpenInputFile(uri), "Error opening arrow file" << bp + file << ur << QString::fromStdString(root_path));
        ArrowBreak(reader, r2, arrow::ipc::RecordBatchFileReader::Open(input), "Error Reading records");

        auto schema = reader->schema();

        ArrowBreak(rowC, r3, reader->CountRows(), "Error reading row count");
        //        qDebug() << "file" << file << schema->fields().size()  << rowC ;
        per_file_counter << rowC;

        for (int record = 0; rowC > 0 && record < reader->num_record_batches(); ++record)
        {
            ArrowGet(rb, r4, reader->ReadRecordBatch(record), "Error Get Record");

            QList<bool> t = double_checker(skiper, rb);
            de_doubler.append(t); // Check for double computations (if forced processings)

            auto well = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("Well"));

            {
                auto array = std::dynamic_pointer_cast<arrow::StringArray>(rb->GetColumnByName("tags"));
                if (array)
                    for (int s = 0; s < array->length(); ++s)
                        if (!tgs.contains(well->GetString(s)))
                            tgs[well->GetString(s)].append(array->GetString(s));
            }

            for (auto f : schema->fields())
            {
                if (!fields.contains(f->name()))
                    fields[f->name()] = f;
                //                if (counts != 0 && !datas.contains(f->name()))
                //                { // Assure that we add non existing cols with empty data if in case
                //                    std::shared_ptr<arrow::Array> ar;

                //                    arrow::NumericBuilder<arrow::FloatType> bldr;
                //                    bldr.AppendNulls(counts);
                //                    bldr.Finish(&ar);

                //                    datas[f->name()].append(ar);
                //                    qDebug() << QString::fromStdString(f->name()) << counts;
                //                }

//                qDebug() << QString::fromStdString(f->name()) << counts;

                datas[f->name()].append(rb->GetColumnByName(f->name()));

                {
                    auto array = std::dynamic_pointer_cast<arrow::FloatArray>(rb->GetColumnByName(f->name()));
                    if (array && (f->name() != "Well" && f->metadata()))
                        for (int s = 0; s < array->length(); ++s)
                            if (de_doubler.last()[s]) // only add if not a double run
                                agg[f->name()][well->GetString(s)].append(array->Value(s));
                }
                lists << f->name();
            }
        }

        //        for (auto& f : datas.keys())
        //        { // Make sure we add data to empty columns
        //            if (!lists.contains(f))
        //            {
        //                if (datas.contains("Well"))
        //                {
        //                  for (auto& wc :   datas["Well"])
        //                        datas[f].append(std::shared_ptr<arrow::NullArray>(new arrow::NullArray(wc->length())));
        //                }
        //            }
        //        }

        counts += rowC;
    }

    for (QMap<std::string, QList<  std::shared_ptr<arrow::Array> > >::iterator wd = datas.begin(), end = datas.end(); wd != end; ++wd)
    {
        for (int i = 0; i < per_file_counter.size() ; ++i)
        {
            //       for (int j = 0; j < per_file_counter.size(); ++j)
            if (i >= wd.value().size())
                datas[wd.key()].insert(i, std::shared_ptr<arrow::NullArray>(new arrow::NullArray(per_file_counter.at(i))));
            if (wd.value().at(i)->length() != per_file_counter.at(i))
                datas[wd.key()].insert(i, std::shared_ptr<arrow::NullArray>(new arrow::NullArray(per_file_counter.at(i))));
//            qDebug() << QString::fromStdString(wd.key()) << wd.value().at(i)->length();
        }
    }


    // Let's construct a double out filtering

    std::vector<std::shared_ptr<arrow::Field> > ff;
    for (auto& item : datas.keys())
        ff.push_back(fields[item]);

    std::vector<std::shared_ptr<arrow::Array> > dat(ff.size());

    int p = 0;

    //    for (auto wd : wrapQMap(datas))
    for (QMap<std::string, QList<  std::shared_ptr<arrow::Array> > >::iterator wd = datas.begin(), end = datas.end(); wd != end; ++wd)
    {
//        qDebug() << QString::fromStdString(wd.key());
        if (is_arrow_type<arrow::FloatArray>(wd.value()))
            concat<arrow::NumericBuilder<arrow::FloatType>, arrow::FloatArray >(wd.value(), dat[p], de_doubler);
        else if (is_arrow_type<arrow::Int16Array>(wd.value()))
            concat<arrow::NumericBuilder<arrow::Int16Type>, arrow::Int16Array >(wd.value(), dat[p], de_doubler);
        else if (is_arrow_type<arrow::StringArray>(wd.value()))
            concat<arrow::StringBuilder, arrow::StringArray >(wd.value(), dat[p], de_doubler);
        else
            qDebug() << p << "No concat!!!";


//        qDebug() << p << QString::fromStdString(wd.key()) << dat[p]->length();

        p++;
    }

    if (dat.size())
    {
        qDebug() << "Generated rows" << dat[0]->length();

        auto schema =
                arrow::schema(ff);
        auto table = arrow::Table::Make(schema, dat);


        std::string uri = out.toStdString();
        std::string root_path;

        auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
        if (!r0.ok())
        {
            qDebug() << "Arrow Error not able to load" << out;
            return;
        }

        auto fs = r0.ValueOrDie();

        auto r1 = fs->OpenOutputStream(uri);

        if (!r1.ok())
        {
            qDebug() << "Arrow Error to Open Stream" << out;
            return;
        }
        auto output = r1.ValueOrDie();
        arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
        //    options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

        auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

        if (!r2.ok())
        {
            qDebug() << "Arrow Unable to make file writer";
            return;
        }

        auto writer = r2.ValueOrDie();

        auto res = writer->WriteTable(*table.get());
        auto res2 = writer->Close();
        auto res3 = output->Close();


        // Let's handle agg
        {
            std::vector<std::shared_ptr<arrow::Field> > ff;
            ff.push_back(fields["Well"]);
            ff.push_back(fields["Plate"]);
            ff.push_back(fields["tags"]);

            for (auto& item : agg.keys())
                ff.push_back(fields[item]);

            std::vector<std::shared_ptr<arrow::Array> > dat(3 + agg.size());

            arrow::StringBuilder wells, plate, tags;

            std::vector<arrow::NumericBuilder<arrow::FloatType> > data(agg.size());

            QList<std::string> ws = agg.first().keys(), fie = agg.keys();

            for (auto& w : ws)
            {
                auto res = wells.Append(w);
                res = plate.Append(plateID.toStdString());
                res = tags.Append(tgs[w]);

                int f = 0;
                for (auto& name : fie)
                    if (fields[name]->metadata())
                    {
                        auto method = fields[name]->metadata()->Contains("Aggregation") ? QString::fromStdString(fields[name]->metadata()->Get("Aggregation").ValueOrDie()) : QString();
                        QList<float>& dat = agg[name][w];
                        res = data[f].Append(Aggregate(dat, method));
                        f++;
                    }
            }

            auto res = wells.Finish(&dat[0]);
            res = plate.Finish(&dat[1]);
            res = tags.Finish(&dat[2]);

            for (int i = 0; i < agg.size(); ++i)
                res = data[i].Finish(&dat[i + 3]);

            auto schema =
                    arrow::schema(ff);
            auto table = arrow::Table::Make(schema, dat);

            QStringList repath = out.split("/");
            repath.last() = QString("ag%1").arg(repath.last());

            qDebug() << "Aggregating to " << (repath.join("/"));
            std::string uri = (repath.join("/").replace("\\", "/").replace("//", "/")).toStdString();
            std::string root_path;

            auto r0 = fs::FileSystemFromUriOrPath(uri, &root_path);
            if (!r0.ok())
            {
                qDebug() << "Arrow Error not able to load" << out;
                return;
            }

            auto fs = r0.ValueOrDie();

            auto r1 = fs->OpenOutputStream(uri);

            if (!r1.ok())
            {
                qDebug() << "Arrow Error to Open Stream" << out;
                return;
            }
            auto output = r1.ValueOrDie();
            arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
            //        options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

            auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

            if (!r2.ok())
            {
                qDebug() << "Arrow Unable to make file writer";
                return;
            }

            auto writer = r2.ValueOrDie();

            auto status = writer->WriteTable(*table.get());
            status = writer->Close();
            status = output->Close();

        }

    }
    // We are lucky enough to get up to here... let's remove the file

    QDir dir(bp);

    for (auto f: files)
    {
        dir.remove(f);
        if (QFileInfo::exists(bp + "/ag" + f))
            dir.remove("ag" + f);
    }


}

#endif // CHECKOUTARROW_H
