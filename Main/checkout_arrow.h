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


namespace fs = arrow::fs;

template<class K,class V>
struct QMapWrapper {
    const QMap<K,V> map;
    QMapWrapper(const QMap<K,V>& map) : map(map) {}
    auto begin() { return map.keyValueBegin(); }
    auto end()   { return map.keyValueEnd();   }
};
template<class K,class V>
QMapWrapper<K,V> wrapQMap(const QMap<K,V>& map) {
    return QMapWrapper<K,V>(map);
}
#define ArrowGet(var, id, call, msg) \
    auto id = call; if (!id.ok()) { qDebug() << msg; return ; } auto var = id.ValueOrDie();


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
typename mytrait<T>::value_type  _get(std::shared_ptr<T> ar, int s)
{
    return ar->Value(s);
}


template<>
mytrait<arrow::StringArray>::value_type _get(std::shared_ptr<arrow::StringArray> ar, int s)
{
    return ar->GetString(s);
}


template <class Bldr, class ColType>
void concat(const QList<std::shared_ptr<arrow::Array> >& list, std::shared_ptr<arrow::Array>& res)
{
    Bldr bldr;

    for (auto& ar: list)
    {
        auto c = std::static_pointer_cast<ColType>(ar);
        for (int s = 0; s < c->length(); ++s)
        {
            if (c->IsValid(s))
                bldr.Append(_get(c, s));
            else
                bldr.AppendNull();
        }

    }


    bldr.Finish(&res);

}


double AggregateSum(QList<float>& f)
{

    if (f.size() == 1) return f.at(0);

    double r = 0;
    foreach(double ff, f)
        r += ff;

    return r;
}
double AggregateMean(QList<float>& f)
{
    if (f.size() == 1) return f.at(0);
    double r = 0;
    foreach(double ff, f)
        r += ff;
    r /= (double)f.size();
    return r;
}




double AggregateMedian(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    std::sort(f.begin(), f.end());
    return f.at(f.size() / 2);
}

double AggregateMin(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::max();
    foreach(double ff, f)
        if (ff < r) r = ff;
    return r;
}

double AggregateMax(QList<float>& f)
{
    if (f.size() < 1) return 0;
    if (f.size() == 1) return f.at(0);
    double r = std::numeric_limits<float>::min();
    foreach(double ff, f)
        if (ff > r) r = ff;
    return r;
}


float Aggregate(QList<float>& f, QString& ag)
{
    if (ag == "Sum") return AggregateSum(f);
    if (ag == "Mean") return AggregateMean(f);
    if (ag == "Median") return AggregateMedian(f);
    if (ag == "Min") return AggregateMin(f);
    if (ag == "Max") return AggregateMax(f);

    qDebug() << "Aggregation error!" << ag << "Not found";
    return -1.;
}


void fuseArrow(QString bp, QStringList files, QString out, QString plateID)
{
    qDebug() << "Fusing" << files << "to" << out;

    QString plateName = out.split("/").last().replace(".fth", "");

    QMap<std::string, QList<  std::shared_ptr<arrow::Array> > > datas;
    QMap<std::string, std::shared_ptr<arrow::Field> >     fields;
    QMap<std::string, QMap<std::string, QList<float> > >  agg;
    QMap<std::string, std::string> tgs;

    int counts = 0;
    for (auto file: files)
    {
        QList<std::string> lists;

        std::string uri = (bp+"/"+file).toStdString(), root_path;
        ArrowGet(fs, r0, fs::FileSystemFromUriOrPath(uri, &root_path), "Arrow File not loading" << file);

        ArrowGet(input, r1, fs->OpenInputFile(uri), "Error opening arrow file" << bp+file);
        ArrowGet(reader, r2, arrow::ipc::RecordBatchFileReader::Open(input), "Error Reading records");

        auto schema = reader->schema();

        ArrowGet(rowC, r3, reader->CountRows(), "Error reading row count");


        for (int record = 0; record < reader->num_record_batches(); ++record)
        {
            ArrowGet(rb, r4, reader->ReadRecordBatch(record), "Error Get Record");

            auto well = std::static_pointer_cast<arrow::StringArray>(rb->GetColumnByName("Well"));

            {
                auto array = std::dynamic_pointer_cast<arrow::StringArray>(rb->GetColumnByName("tags"));
                if (array )
                    for (int s = 0; s < array->length(); ++s)
                        if (!tgs.contains(well->GetString(s)))
                            tgs[well->GetString(s)].append(array->GetString(s));
            }

            for (auto f : schema->fields())

            {
                if (!fields.contains(f->name()))
                    fields[f->name()] = f;
                if (counts != 0 && !datas.contains(f->name()))
                { // Assure that we add non existing cols with empty data if in case
                    std::shared_ptr<arrow::Array> ar;

                    arrow::NumericBuilder<arrow::FloatType> bldr;
                    bldr.AppendNulls(counts);
                    bldr.Finish(&ar);
                    datas[f->name()].append(ar);
                }
                datas[f->name()].append(rb->GetColumnByName(f->name()));

                {
                    auto array = std::dynamic_pointer_cast<arrow::FloatArray>(rb->GetColumnByName(f->name()));
                    if (array &&   (f->name() != "Well" && f->metadata()))
                        for (int s = 0; s < array->length(); ++s)
                            agg[f->name()][well->GetString(s)].append(array->Value(s));

                }


                lists << f->name();
            }
        }

        for (auto& f: datas.keys())
        { // Make sure we add data to empty columns
            if (!lists.contains(f))
                datas[f].append(std::shared_ptr<arrow::NullArray>(new arrow::NullArray(rowC)));
        }

        counts += rowC;
    }



    std::vector<std::shared_ptr<arrow::Field> > ff;
    for (auto& item: datas.keys())
        ff.push_back(fields[item]);


    std::vector<std::shared_ptr<arrow::Array> > dat(ff.size());

    int p = 0;
    for (auto wd: wrapQMap(datas))
    {
        if (std::dynamic_pointer_cast<arrow::FloatArray>(wd.second.first()))
            concat<arrow::NumericBuilder<arrow::FloatType>, arrow::FloatArray >(wd.second, dat[p]);
        else if (std::dynamic_pointer_cast<arrow::Int16Array>(wd.second.first()))
            concat<arrow::NumericBuilder<arrow::Int16Type> , arrow::Int16Array >(wd.second, dat[p]);
        else if (std::dynamic_pointer_cast<arrow::StringArray>(wd.second.first()))
            concat<arrow::StringBuilder, arrow::StringArray > (wd.second, dat[p]);
        p++;
    }


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
    //        options.codec = arrow::util::Codec::Create(arrow::Compression::LZ4).ValueOrDie(); //std::make_shared<arrow::util::Codec>(codec);

    auto r2 = arrow::ipc::MakeFileWriter(output.get(), table->schema(), options);

    if (!r2.ok())
    {
        qDebug() << "Arrow Unable to make file writer";
        return;
    }

    auto writer = r2.ValueOrDie();

    writer->WriteTable(*table.get());
    writer->Close();

    // Let's handle agg
    {
        std::vector<std::shared_ptr<arrow::Field> > ff;
        ff.push_back(fields["Well"]);
        ff.push_back(fields["Plate"]);
        ff.push_back(fields["tags"]);

        for (auto& item: agg.keys())
            ff.push_back(fields[item]);

        std::vector<std::shared_ptr<arrow::Array> > dat(3+agg.size());

        arrow::StringBuilder wells, plate, tags;

        std::vector<arrow::NumericBuilder<arrow::FloatType> > data(agg.size());

        QList<std::string> ws = agg.first().keys(), fie = agg.keys();

        for (auto& w: ws)
        {
            wells.Append(w);
            plate.Append(plateID.toStdString());
            tags.Append(tgs[w]);

            int f = 0;
            for (auto& name: fie)
                if (fields[name]->metadata())
                {
                    auto method = fields[name]->metadata()->Contains("Aggregation") ? QString::fromStdString(fields[name]->metadata()->Get("Aggregation").ValueOrDie()) : QString();
                    QList<float>& dat = agg[name][w];
                    data[f].Append(Aggregate(dat, method));
                    f++;
                }
        }

        wells.Finish(&dat[0]);
        plate.Finish(&dat[1]);
        tags.Finish(&dat[2]);

        for (int i = 0; i < agg.size(); ++i)
            data[i].Finish(&dat[i+3]);

        auto schema =
                arrow::schema(ff);
        auto table = arrow::Table::Make(schema, dat);

        QStringList repath = out.split("/");
        repath.last() = QString("ag%1").arg(repath.last());

        qDebug() << "Aggregating to " << (repath.join("/"));
        std::string uri = (repath.join("/")).toStdString();
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

        writer->WriteTable(*table.get());
        writer->Close();

    }


    // We are lucky enough to get up to here... let's remove the file

    QDir dir(bp);

    for (auto f: files)
        dir.remove(f);


}

#endif // CHECKOUTARROW_H
