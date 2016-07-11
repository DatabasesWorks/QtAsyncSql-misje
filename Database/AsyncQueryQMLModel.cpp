#include "AsyncQueryQMLModel.h"
#include "AsyncQuery.h"
using namespace Database;

static const int firstRole = Qt::UserRole + 1;

AsyncQueryQMLModel::AsyncQueryQMLModel(QObject *parent)
	: QAbstractTableModel(parent)
	, _aQuery(new AsyncQuery(this))
{
	connect(_aQuery, &AsyncQuery::execDone, this, &AsyncQueryQMLModel::onExecDone);
}

AsyncQuery *AsyncQueryQMLModel::asyncQuery() const
{
	return _aQuery;
}

QString AsyncQueryQMLModel::queryString() const
{
	return _aQuery->query();
}

QSqlError AsyncQueryQMLModel::error() const
{
	return _res.error();
}

void AsyncQueryQMLModel::startExec(const QString &query)
{
	_aQuery->startExec(query);
}

void AsyncQueryQMLModel::clear()
{
	beginResetModel();
	_res = {};
	_roleNames.clear();
	_roleIDs.clear();
	setColumnNames({});
	endResetModel();
}

int AsyncQueryQMLModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return _res.count();
}

int AsyncQueryQMLModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return _res.headRecord().count();
}

QVariant AsyncQueryQMLModel::data(const QModelIndex &index, int role) const
{
	if (role >= firstRole && role < firstRole + columnCount())
		return _res.value(index.row(), role - firstRole);

	return {};
}

QVariant AsyncQueryQMLModel::data(int row, const QString &role) const
{
	if (row >= 0 && row < rowCount() && _roleIDs.contains(role))
		return _res.value(row, _roleIDs.value(role) - firstRole);

	return {};
}

QHash<int, QByteArray> AsyncQueryQMLModel::roleNames() const
{
	return _roleNames;
}

void AsyncQueryQMLModel::setQueryString(const QString &query)
{
	if (query == asyncQuery()->query())
		return;

	clear();
	asyncQuery()->prepare(query);
	emit queryStringChanged(query);
}

void AsyncQueryQMLModel::bindValue(const QString &name, const QVariant &value)
{
	asyncQuery()->bindValue(name, value);
}

void AsyncQueryQMLModel::exec()
{
	asyncQuery()->startExec();
}

void AsyncQueryQMLModel::onExecDone(const Database::AsyncQueryResult &result)
{
	beginResetModel();
	_res = result;
	updateRoles();
	endResetModel();

	if (result.isValid())
		emit querySucceeded();
	else
		emit queryFailed(result.error().text());
}

void AsyncQueryQMLModel::updateRoles()
{
	_roleNames.clear();
	_roleIDs.clear();
	auto record = _res.headRecord();
	QStringList columnNames;
	for (int i = 0; i < record.count(); ++i)
	{
		auto name = record.fieldName(i);
		auto id = firstRole + i;
		_roleNames[id] = name.toUtf8();
		_roleIDs[name] = id;
		columnNames << name;
	}

	setColumnNames(columnNames);
}

void AsyncQueryQMLModel::setColumnNames(const QStringList &columnNames)
{
	if (columnNames == _columnNames)
		return;

	_columnNames = columnNames;
	emit columnNamesChanged(_columnNames);
}
