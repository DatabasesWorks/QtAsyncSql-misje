#pragma once
#include <QAbstractTableModel>
#include "AsyncQueryResult.h"

namespace Database
{
class AsyncQuery;

class AsyncQueryQMLModel : public QAbstractTableModel
{
	Q_OBJECT
	Q_PROPERTY(QString query READ queryString WRITE setQueryString NOTIFY
			queryStringChanged)
	Q_PROPERTY(QStringList columnNames MEMBER _columnNames NOTIFY columnNamesChanged)

signals:
	void queryStringChanged(const QString &queryString);
	void columnNamesChanged(const QStringList &columnNames);
	void querySucceeded();
	void queryFailed(const QString &errorMessage);

public:
	explicit AsyncQueryQMLModel(QObject *parent = nullptr);

	AsyncQuery *asyncQuery() const;
	QString queryString() const;
	QSqlError error() const;
	void startExec(const QString &query);
	void clear();

	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	Q_INVOKABLE QVariant data(int row, const QString &role) const;
	QHash<int, QByteArray> roleNames() const override;

	void setQueryString(const QString &query);

public slots:
	void bindValue(const QString &name, const QVariant &value);
	void exec();

private:
	void onExecDone(const Database::AsyncQueryResult &result);
	void updateRoles();
	void setColumnNames(const QStringList &columnNames);

	QHash<int, QByteArray> _roleNames;
	QHash<QString, int> _roleIDs;
	QStringList _columnNames;
	AsyncQueryResult _res;
	AsyncQuery *_aQuery;
};
};
