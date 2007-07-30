/* This  is part of KDevelop
    Copyright 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "duchain.h"
#include "duchainlock.h"

#include <kglobal.h>

#include "editorintegrator.h"

#include "topducontext.h"
#include "parsingenvironment.h"

namespace KDevelop
{

class DUChainPrivate
{
public:
  DUChain instance;
  DUChainLock lock;
  QMap<IdentifiedFile, TopDUContext*> m_chains;
  QList<DUChainObserver*> m_observers;
  QMap<int,ParsingEnvironmentManager*> m_managers;
};

K_GLOBAL_STATIC(DUChainPrivate, sdDUChainPrivate)


DUChain::DUChain()
{
}

DUChain::~DUChain()
{
}

DUChain* DUChain::self()
{
  return &sdDUChainPrivate->instance;
}

DUChainLock* DUChain::lock()
{
  return &sdDUChainPrivate->lock;
}

void DUChain::updateContextEnvironment( TopDUContext* context, ParsingEnvironmentFile* file ) {
  ENSURE_CHAIN_WRITE_LOCKED

  removeFromEnvironmentManager( context );
  context->setParsingEnvironmentFile( file );
  addToEnvironmentManager( context );
}


void DUChain::removeDocumentChain( const  IdentifiedFile& document )
{
  ENSURE_CHAIN_WRITE_LOCKED

  kDebug() << "duchain: removing document " << document.toString() << endl;
  sdDUChainPrivate->m_chains.remove(document);
}

void DUChain::addDocumentChain( const IdentifiedFile& document, TopDUContext * chain )
{
  ENSURE_CHAIN_WRITE_LOCKED

  kDebug() << "duchain: adding document " << document.toString() << endl;
  Q_ASSERT(chain);
  sdDUChainPrivate->m_chains.insert(document, chain);
  chain->setInDuChain(true);
  addToEnvironmentManager(chain);
}

void DUChain::addToEnvironmentManager( TopDUContext * chain ) {
  ENSURE_CHAIN_WRITE_LOCKED

  ParsingEnvironmentFilePointer file = chain->parsingEnvironmentFile();
  if( !file )
    return; //We don't need to manage

  QMap<int,ParsingEnvironmentManager*>::const_iterator it = sdDUChainPrivate->m_managers.find(file->type());

  if( it != sdDUChainPrivate->m_managers.end() ) {
    (*it)->addFile( file.data() );
  } else {
    //No manager available for the type
  }
}

void DUChain::removeFromEnvironmentManager( TopDUContext * chain ) {
  ENSURE_CHAIN_WRITE_LOCKED

  ParsingEnvironmentFilePointer file = chain->parsingEnvironmentFile();
  if( !file )
    return; //We don't need to manage

  QMap<int,ParsingEnvironmentManager*>::const_iterator it = sdDUChainPrivate->m_managers.find(file->type());

  if( it != sdDUChainPrivate->m_managers.end() ) {
    (*it)->removeFile( file.data() );
  } else {
    //No manager available for the type
  }
}

TopDUContext* DUChain::chainForDocument(const KUrl& document) const {
  return chainForDocument( IdentifiedFile(document) );
}

TopDUContext * DUChain::chainForDocument( const IdentifiedFile & document ) const
{
  if (!document.identity()) {
    // Match any parsed version of this document
    QMap<IdentifiedFile, TopDUContext*>::Iterator it = sdDUChainPrivate->m_chains.lowerBound(document);
    if (it != sdDUChainPrivate->m_chains.constEnd())
      if (it.key().url() == document.url())
        return it.value();

  } else if (sdDUChainPrivate->m_chains.contains(document))
    return sdDUChainPrivate->m_chains[document];

  return 0;
}

QList<TopDUContext*> DUChain::chainsForDocument(const KUrl& document) const
{
  QList<TopDUContext*> chains;

  // Match all parsed versions of this document
  for (QMap<IdentifiedFile, TopDUContext*>::Iterator it = sdDUChainPrivate->m_chains.lowerBound(IdentifiedFile(document)); it != sdDUChainPrivate->m_chains.constEnd(); ++it) {
    if (it.key().url() == document.url())
      chains << it.value();
    else
      break;
  }

  return chains;
}

TopDUContext* DUChain::chainForDocument( const KUrl& document, const ParsingEnvironment* environment ) const {
  QMap<int,ParsingEnvironmentManager*>::const_iterator it = sdDUChainPrivate->m_managers.find(environment->type());
  if( it != sdDUChainPrivate->m_managers.end() ) {
    ParsingEnvironmentFilePointer file( (*it)->find(document, environment) );
    if( !file )
      return 0;

      return chainForDocument( file->identity() );
  } else {
    //No manager available for the type
    return chainForDocument( document );
  }
}

void DUChain::clear()
{
  DUChainWriteLocker writeLock(lock());
  foreach (TopDUContext* context, sdDUChainPrivate->m_chains) {
    KDevelop::EditorIntegrator::releaseTopRange(context->textRangePtr());
    delete context;
  }

  foreach( ParsingEnvironmentManager* manager, sdDUChainPrivate->m_managers )
    manager->clear();

  sdDUChainPrivate->m_chains.clear();
}

QList< DUChainObserver * > DUChain::observers() const
{
  ENSURE_CHAIN_READ_LOCKED

  return sdDUChainPrivate->m_observers;
}

void DUChain::addObserver(DUChainObserver * observer)
{
  ENSURE_CHAIN_WRITE_LOCKED

  Q_ASSERT(!sdDUChainPrivate->m_observers.contains(observer));
  sdDUChainPrivate->m_observers.append(observer);
}

void DUChain::removeObserver(DUChainObserver * observer)
{
  ENSURE_CHAIN_WRITE_LOCKED

  sdDUChainPrivate->m_observers.removeAll(observer);
}

///This mutex is used to make sure that only one observer-callback is called at a time
///That is needed because addDeclaration(..) can be called from multiple threads simultaneuously, triggering multiple observer-callbacks.
QMutex duChainObserverMutex(QMutex::Recursive);

void DUChain::contextChanged(DUContext* context, DUChainObserver::Modification change, DUChainObserver::Relationship relationship, DUChainBase* relatedObject)
{
  if( !sdDUChainPrivate->m_observers.isEmpty() )
  {
    QMutexLocker l(&duChainObserverMutex);
    foreach (DUChainObserver* observer, self()->observers())
      observer->contextChanged(context, change, relationship, relatedObject);
  }
}

void DUChain::declarationChanged(Declaration* declaration, DUChainObserver::Modification change, DUChainObserver::Relationship relationship, DUChainBase* relatedObject)
{
  if( !sdDUChainPrivate->m_observers.isEmpty() )
  {
    QMutexLocker l(&duChainObserverMutex);
    foreach (DUChainObserver* observer, self()->observers())
      observer->declarationChanged(declaration, change, relationship, relatedObject);
  }
}

void DUChain::definitionChanged(Definition* definition, DUChainObserver::Modification change, DUChainObserver::Relationship relationship, DUChainBase* relatedObject)
{
  if( !sdDUChainPrivate->m_observers.isEmpty() )
  {
    QMutexLocker l(&duChainObserverMutex);
    foreach (DUChainObserver* observer, self()->observers())
      observer->definitionChanged(definition, change, relationship, relatedObject);
  }
}

void DUChain::useChanged(Use* use, DUChainObserver::Modification change, DUChainObserver::Relationship relationship, DUChainBase* relatedObject)
{
  if( !sdDUChainPrivate->m_observers.isEmpty() )
  {
    QMutexLocker l(&duChainObserverMutex);
    foreach (DUChainObserver* observer, self()->observers())
      observer->useChanged(use, change, relationship, relatedObject);
  }
}

void DUChain::addParsingEnvironmentManager( ParsingEnvironmentManager* manager ) {
  ENSURE_CHAIN_WRITE_LOCKED
  Q_ASSERT( sdDUChainPrivate->m_managers.find(manager->type()) == sdDUChainPrivate->m_managers.end() ); //If this fails, there may be multiple managers with the same type, which is wrong

  sdDUChainPrivate->m_managers[manager->type()] = manager;
  }

void DUChain::removeParsingEnvironmentManager( ParsingEnvironmentManager* manager ) {
  ENSURE_CHAIN_WRITE_LOCKED
  QMap<int,ParsingEnvironmentManager*>::iterator it = sdDUChainPrivate->m_managers.find(manager->type());

  if( it != sdDUChainPrivate->m_managers.end() ) {
    Q_ASSERT( *it == manager ); //If this fails, there may be multiple managers with the same type, which is wrong
    sdDUChainPrivate->m_managers.erase(it);
  }
}

QList<KUrl> DUChain::documents() const
{
  QList<KUrl> ret;
  foreach (const IdentifiedFile& file, sdDUChainPrivate->m_chains.keys()) {
    ret << file.url();
  }
  return ret;
}

}

#include "duchain.moc"

// kate: space-indent on; indent-width 2; tab-width: 4; replace-tabs on; auto-insert-doxygen on
