#ifndef CHECKOUTPYTHONENV_H
#define CHECKOUTPYTHONENV_H




void parse_python_conf(QFile& loadFile, QProcessEnvironment& python_config)
{
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open config file, skipping");
    }
    else
    {

        QByteArray saveData = loadFile.readAll();

        QCborMap pc =  QCborMap::fromJsonObject(
                    QJsonDocument::fromJson(saveData).object());

        python_config = QProcessEnvironment::systemEnvironment();



        for (auto k: pc)
        {

            QStringList vals = k.second.toString().split(";"), nxt;
            if (k.second.toString().contains(";"))
                vals = k.second.toString().split(";");
            else if (k.second.toString().contains(":"))
                vals = k.second.toString().split(":");

            for (auto& v : vals)
            {
                if (v[0]=='$')
                {
                    QString s = v.replace("$","");
                    if (python_config.contains(s) )
                        nxt << python_config.value(s);
                }
                else
                    nxt << v;
               }
            python_config.insert(k.first.toString(), nxt.join(";"));
        }

    }
}


#endif // CHECKOUTPYTHONENV_H
