
//
// Example EPICS CA server
// (asynchrronous process variable)
//

#include "exServer.h"

//
// exAsyncPV::read()
// (virtual replacement for the default)
//
caStatus exAsyncPV::read (const casCtx &ctx, gdd &valueIn)
{
	exAsyncReadIO	*pIO;
	
	if (this->simultAsychIOCount>=maxSimultAsyncIO) {
		return S_casApp_postponeAsyncIO;
	}

	this->simultAsychIOCount++;

	pIO = new exAsyncReadIO ( ctx, *this, valueIn );
	if (!pIO) {
		return S_casApp_noMemory;
	}

    return S_casApp_asyncCompletion;
}

//
// exAsyncPV::write()
// (virtual replacement for the default)
//
caStatus exAsyncPV::write ( const casCtx &ctx, const gdd &valueIn )
{
	exAsyncWriteIO *pIO;
	
	if ( this->simultAsychIOCount >= maxSimultAsyncIO ) {
		return S_casApp_postponeAsyncIO;
	}

	this->simultAsychIOCount++;

	pIO = new exAsyncWriteIO ( ctx, *this, valueIn );
	if ( ! pIO ) {
		return S_casApp_noMemory;
	}

	return S_casApp_asyncCompletion;
}

//
// exAsyncWriteIO::exAsyncWriteIO()
//
exAsyncWriteIO::exAsyncWriteIO ( const casCtx &ctxIn, exAsyncPV &pvIn, 
    const gdd &valueIn ) :
	casAsyncWriteIO ( ctxIn ), pv ( pvIn ), 
        timer ( pvIn.getCAS()->createTimer () ), pValue(valueIn)
{
    this->timer.start ( *this, 0.1 );
}

//
// exAsyncWriteIO::~exAsyncWriteIO()
//
exAsyncWriteIO::~exAsyncWriteIO()
{
	this->pv.removeIO();
    if ( this->pv.getCAS() ) {
        this->timer.destroy ();
    }
}

//
// exAsyncWriteIO::expire()
// (a virtual function that runs when the base timer expires)
//
epicsTimerNotify::expireStatus exAsyncWriteIO::expire ( const epicsTime & currentTime ) 
{
	caStatus status;
	status = this->pv.update ( this->pValue );
	this->postIOCompletion ( status );
    return noRestart;
}

//
// exAsyncReadIO::exAsyncReadIO()
//
exAsyncReadIO::exAsyncReadIO ( const casCtx &ctxIn, exAsyncPV &pvIn, 
    gdd &protoIn ) :
	casAsyncReadIO ( ctxIn ), pv ( pvIn ), 
        timer ( pvIn.getCAS()->createTimer() ), pProto ( protoIn )
{
    this->timer.start ( *this, 0.1 );
}

//
// exAsyncReadIO::~exAsyncReadIO()
//
exAsyncReadIO::~exAsyncReadIO()
{
	this->pv.removeIO ();
    if ( this->pv.getCAS() ) {
        this->timer.destroy ();
    }
}


//
// exAsyncReadIO::expire()
// (a virtual function that runs when the base timer expires)
//
epicsTimerNotify::expireStatus exAsyncReadIO::expire ( const epicsTime & currentTime )
{
	caStatus status;

	//
	// map between the prototype in and the
	// current value
	//
	status = this->pv.exPV::readNoCtx ( this->pProto );

	//
	// post IO completion
	//
	this->postIOCompletion ( status, *this->pProto );

    return noRestart;
}

