#include "datacadence.h"
#include <qvector.h>
#include <qmutex.h>
#include <qreadwritelock.h>

class DataCadence::PrivateData
{
public:
    PrivateData():
        boundingRect( 1.0, 1.0, -2.0, -2.0 ) // invalid
    {
        values.reserve( 1000 );
    }

    inline void append( const QPointF &sample )
    {
        values.append( sample );

        // adjust the bounding rectangle

        if ( boundingRect.width() < 0 || boundingRect.height() < 0 )
        {
            boundingRect.setRect( sample.x(), sample.y(), 0.0, 0.0 );
        }
        else
        {
            boundingRect.setRight( sample.x() );

            if ( sample.y() > boundingRect.bottom() )
                boundingRect.setBottom( sample.y() );

            if ( sample.y() < boundingRect.top() )
                boundingRect.setTop( sample.y() );
        }
    }

    inline void clear() {
        values.clear();
    }

    QReadWriteLock lock;

    QVector<QPointF> values;
    QRectF boundingRect;

    QMutex mutex; // protecting pendingValues
    QVector<QPointF> pendingValues;
};


//////////////////////////////////////////////////////////////////////////////////////////////
DataCadence::DataCadence()
{
    d_data = new PrivateData();
}

DataCadence::~DataCadence()
{
    delete d_data;
}

int DataCadence::size() const
{
    return d_data->values.size();
}

QPointF DataCadence::value( int index ) const
{
    return d_data->values[index];
}

QRectF DataCadence::boundingRect() const
{
    return d_data->boundingRect;
}

void DataCadence::lock()
{
    d_data->lock.lockForRead();
}

void DataCadence::unlock()
{
    d_data->lock.unlock();
}

void DataCadence::append( const QPointF &sample )
{
    d_data->mutex.lock();
    d_data->pendingValues += sample;

    const bool isLocked = d_data->lock.tryLockForWrite();
    if ( isLocked )
    {
        const int numValues = d_data->pendingValues.size();
        const QPointF *pendingValues = d_data->pendingValues.data();

        for ( int i = 0; i < numValues; i++ )
            d_data->append( pendingValues[i] );

        d_data->pendingValues.clear();

        d_data->lock.unlock();
    }

    d_data->mutex.unlock();
}

void DataCadence::clearStaleValues( double limit )
{
    d_data->lock.lockForWrite();

    d_data->boundingRect = QRectF( 1.0, 1.0, -2.0, -2.0 ); // invalid

    const QVector<QPointF> values = d_data->values;
    d_data->values.clear();
    d_data->values.reserve( values.size() );

    int index;
    for ( index = values.size() - 1; index >= 0; index-- )
    {
        if ( values[index].x() < limit )
            break;
    }

    if ( index > 0 )
        d_data->append( values[index++] );

    while ( index < values.size() - 1 )
        d_data->append( values[index++] );

    d_data->lock.unlock();
}

////////////////////////////////////////////////////////////////////////////////////
void DataCadence::clearData()
{
    d_data->clear();
}


////////////////////////////////////////////////////////////////////////////////////
DataCadence &DataCadence::instance()
{
    static DataCadence valueVector;
    return valueVector;
}
