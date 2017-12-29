#include "AsyncQuery.h"
#include "ConnectionManager.h"

#include <QRunnable>
#include <QSqlQuery>
#include <QThreadPool>
#include <QQueue>


namespace Database {

class SqlTaskPrivate : public QRunnable
{
public:
	SqlTaskPrivate(AsyncQuery *instance, AsyncQuery::QueuedQuery query,
				   ulong delayMs = 0);

	void run() override;

private:
	AsyncQuery* _instance;
	AsyncQuery::QueuedQuery _query;
	ulong _delayMs;

};

SqlTaskPrivate::SqlTaskPrivate(AsyncQuery *instance, AsyncQuery::QueuedQuery query,
							   ulong delayMs)
	: _instance(instance)
	, _query(query)
	, _delayMs(delayMs)
{
}

void SqlTaskPrivate::run()
{
	Q_ASSERT(_instance);

	AsyncQueryResult result;
	ConnectionManager* conmgr = ConnectionManager::instance();
	if (!conmgr->connectionExists()) {
		if (!conmgr->open(&result._error))
		{
			result._queryString = _query.query;
			_instance->taskCallback(result);
			return;
		}
	}

	QSqlDatabase db = conmgr->threadConnection();
	if (!db.isOpen() && !db.open())
	{
		result._queryString = _query.query;
		result._error = db.lastError();
		_instance->taskCallback(result);
		return;
	}

	//delay query
	if (_delayMs > 0) {
		QThread::currentThread()->msleep(_delayMs);
	}

	QSqlQuery query = QSqlQuery(db);
	bool succ = true;
	if (_query.isPrepared) {
		succ = query.prepare(_query.query);
		//bind values
		QMapIterator<QString, QVariant> i(_query.boundValues);
		while (i.hasNext()) {
			i.next();
			query.bindValue(i.key(), i.value());
		}
	}
	if (succ) {
		if (_query.isPrepared) {
			if (_query.isBatch) {
				query.execBatch();
			}
			else {
				query.exec();
			}
		}
		else {
			query.exec(_query.query);
		}
	}

	result._queryString = query.executedQuery();
	result._record = query.record();
	result._error = query.lastError();
	result._lastInsertId = query.lastInsertId();
	result._numRowsAffected = query.numRowsAffected();
	int cols = result._record.count();

	while (query.next()) {
		QVector<QVariant> currow(cols);

		for (int ii = 0; ii < cols; ii++) {
			if (query.isNull(ii)) {
				currow[ii] = QVariant();
			}
			else {
				currow[ii] = query.value(ii);
			}
		}
		result._data.append(currow);
	}

	//send result
	_instance->taskCallback(result);
}

/****************************************************************************************/
/*                                          AsyncQuery                                  */
/****************************************************************************************/


AsyncQuery::AsyncQuery(QObject* parent /* = nullptr */)
	: QObject(parent), logger("Database.AsyncQuery")
	, _deleteOnDone(false)
	, _delayMs(0)
	, _mode(Mode_Parallel)
	, _taskCnt(0)
	, _isBatch(false)
{
}

AsyncQuery::~AsyncQuery()
{
}

void AsyncQuery::setMode(AsyncQuery::Mode mode)
{
	QMutexLocker locker(&_mutex);
	_mode = mode;
}

AsyncQuery::Mode AsyncQuery::mode()
{
	QMutexLocker locker(&_mutex);
	return _mode;
}

bool AsyncQuery::isRunning() const
{
	QMutexLocker lock(&_mutex);
	return (_taskCnt > 0);
}

QString AsyncQuery::query() const
{
	return _curQuery.query;
}

AsyncQueryResult AsyncQuery::result() const
{
	QMutexLocker lock(&_mutex);
	return _result;
}

void AsyncQuery::prepare(const QString &query)
{
	_curQuery.query = query;
}

bool AsyncQuery::bindValue(const QString &placeholder, const QVariant &val)
{
	if (_isBatch)
		return false;

	_curQuery.boundValues[placeholder] = val;
	return true;
}

bool AsyncQuery::bindBatchValue(const QString &placeholder, const QVariantList &values)
{
	if (values.isEmpty())
		return false;

	QVariant::Type type = values.first().type();
	for (const auto &val : values)
		if (val.type() != type)
			return false;

	bindValue(placeholder, values);
	_isBatch = true;
	return true;
}

void AsyncQuery::startExec()
{
	_curQuery.isPrepared = true;
	_curQuery.isBatch = _isBatch;
	startExecIntern();
}

void AsyncQuery::startExec(const QString &query)
{
	_curQuery.isPrepared = false;
	_curQuery.query = query;
	startExecIntern();

}

bool AsyncQuery::waitDone(ulong msTimout)
{
	QMutexLocker lock(&_mutex);
	if (_taskCnt > 0)
		return _waitcondition.wait(&_mutex, msTimout);
	else
		return true;
}

void AsyncQuery::startExecOnce(const QString &query, QObject *receiver, const char *member)
{
	AsyncQuery *q = new AsyncQuery();
	q->_deleteOnDone = true;
	connect(q, SIGNAL(execDone(Database::AsyncQueryResult)),
			receiver, member);
	q->startExec(query);
}

void AsyncQuery::setDelayMs(ulong ms)
{
	QMutexLocker locker(&_mutex);
	_delayMs = ms;
}

void AsyncQuery::startExecIntern()
{
	QMutexLocker lock(&_mutex);
	if (_mode == Mode_Parallel) {
		QThreadPool* pool = QThreadPool::globalInstance();
		SqlTaskPrivate* task = new SqlTaskPrivate(this, _curQuery, _delayMs);
		incTaskCount();
		pool->start(task);
	} else {
		if (_taskCnt == 0) {
			QThreadPool* pool = QThreadPool::globalInstance();
			SqlTaskPrivate* task = new SqlTaskPrivate(this, _curQuery, _delayMs);
			incTaskCount();
			pool->start(task);
		} else {
			if (_mode == Mode_Fifo) {
				_ququ.enqueue(_curQuery);
			} else {
				_ququ.clear();
				_ququ.enqueue(_curQuery);
			}
		}
	}
}

void AsyncQuery::incTaskCount()
{
	bool busyChanged = _taskCnt == 0;
	_taskCnt++;
	if (busyChanged)
		emit AsyncQuery::busyChanged(true);
}

void AsyncQuery::decTaskCount()
{
	bool busyChanged = _taskCnt == 1;
	_taskCnt--;
	if (busyChanged)
		emit AsyncQuery::busyChanged(false);
}

void AsyncQuery::taskCallback(const AsyncQueryResult& result)
{
	_mutex.lock();
	Q_ASSERT(_taskCnt > 0);
	_result = result;
	if (_mode != Mode_Parallel && !_ququ.isEmpty()) {
		//start next query if queue not empty
		QueuedQuery query = _ququ.dequeue();
		QThreadPool* pool = QThreadPool::globalInstance();
		SqlTaskPrivate* task = new SqlTaskPrivate(this, query, _delayMs);
		pool->start(task);
	} else {
		decTaskCount();
	}

	_waitcondition.wakeAll();
	_mutex.unlock();

	emit execDone(result);

	if (_deleteOnDone) {
		// note delete later should be thread save
		deleteLater();
	}
}
}
