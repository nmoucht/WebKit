/*
 * Copyright (C) 2014, 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SQLiteIDBCursor.h"

#include "IDBCursorInfo.h"
#include "IDBGetResult.h"
#include "IDBSerialization.h"
#include "Logging.h"
#include "SQLiteIDBBackingStore.h"
#include "SQLiteIDBTransaction.h"
#include "SQLiteStatement.h"
#include <sqlite3.h>
#include <wtf/TZoneMallocInlines.h>
#include <wtf/text/MakeString.h>

namespace WebCore {
namespace IDBServer {

static const size_t prefetchLimit = 256;
static const size_t prefetchSizeLimit = 1 * MB;

WTF_MAKE_TZONE_ALLOCATED_IMPL(SQLiteIDBCursor);

std::unique_ptr<SQLiteIDBCursor> SQLiteIDBCursor::maybeCreate(SQLiteIDBTransaction& transaction, const IDBCursorInfo& info)
{
    auto cursor = makeUnique<SQLiteIDBCursor>(transaction, info);

    if (!cursor->establishStatement())
        return nullptr;

    if (!cursor->advance(1))
        return nullptr;

    return cursor;
}

std::unique_ptr<SQLiteIDBCursor> SQLiteIDBCursor::maybeCreateBackingStoreCursor(SQLiteIDBTransaction& transaction, IDBObjectStoreIdentifier objectStoreID, std::optional<IDBIndexIdentifier> indexID, const IDBKeyRangeData& range)
{
    auto cursor = makeUnique<SQLiteIDBCursor>(transaction, objectStoreID, indexID, range);

    if (!cursor->establishStatement())
        return nullptr;

    if (!cursor->advance(1))
        return nullptr;

    return cursor;
}

SQLiteIDBCursor::SQLiteIDBCursor(SQLiteIDBTransaction& transaction, const IDBCursorInfo& info)
    : m_transaction(&transaction)
    , m_cursorIdentifier(info.identifier())
    , m_objectStoreID(info.objectStoreIdentifier())
    , m_indexID(info.sourceIndexIdentifier())
    , m_cursorDirection(info.cursorDirection())
    , m_cursorType(info.cursorType())
    , m_keyRange(info.range())
    , m_boundID(m_objectStoreID)
{
}

SQLiteIDBCursor::SQLiteIDBCursor(SQLiteIDBTransaction& transaction, IDBObjectStoreIdentifier objectStoreID, std::optional<IDBIndexIdentifier> indexID, const IDBKeyRangeData& range)
    : m_transaction(&transaction)
    , m_cursorIdentifier(transaction.transactionIdentifier())
    , m_objectStoreID(objectStoreID)
    , m_indexID(indexID)
    , m_cursorDirection(IndexedDB::CursorDirection::Next)
    , m_cursorType(IndexedDB::CursorType::KeyAndValue)
    , m_keyRange(range)
    , m_boundID(m_objectStoreID)
    , m_backingStoreCursor(true)
{
}

SQLiteIDBCursor::~SQLiteIDBCursor()
{
    if (m_backingStoreCursor)
        m_transaction->closeCursor(*this);
}

SQLiteIDBTransaction* SQLiteIDBCursor::transaction() const
{
    return m_transaction.get();
}

void SQLiteIDBCursor::currentData(IDBGetResult& result, const std::optional<IDBKeyPath>& keyPath, ShouldIncludePrefetchedRecords shouldIncludePrefetchedRecords)
{
    ASSERT(!m_fetchedRecords.isEmpty());

    auto& currentRecord = m_fetchedRecords.first();
    if (currentRecord.completed) {
        ASSERT(!currentRecord.errored);
        result = { };
        return;
    }

    if (shouldIncludePrefetchedRecords == ShouldIncludePrefetchedRecords::No) {
        result = { currentRecord.record.key, currentRecord.record.primaryKey, IDBValue(currentRecord.record.value), keyPath };
        return;
    }

    Vector<IDBCursorRecord> prefetchedRecords;
    prefetchedRecords.reserveInitialCapacity(m_fetchedRecords.size() - 1);
    bool isFirst = true;
    for (auto& record : m_fetchedRecords) {
        if (record.isTerminalRecord())
            break;
        if (isFirst) {
            isFirst = false;
            continue;
        }
        prefetchedRecords.append(record.record);
    }
    prefetchedRecords.shrinkToFit();

    result = { currentRecord.record.key, currentRecord.record.primaryKey, IDBValue(currentRecord.record.value), keyPath, WTFMove(prefetchedRecords) };
}

static String buildPreIndexStatement(bool isDirectionNext)
{
    return makeString("SELECT rowid, key, value FROM IndexRecords WHERE indexID = ? AND key = CAST(? AS TEXT) AND value "_s,
        isDirectionNext ? '>' : '<', " CAST(? AS TEXT) ORDER BY value"_s, isDirectionNext ? ""_s : " DESC"_s, ';');
}

static String buildIndexStatement(const IDBKeyRangeData& keyRange, IndexedDB::CursorDirection cursorDirection)
{
    return makeString("SELECT rowid, key, value FROM IndexRecords WHERE indexID = ? AND key "_s,
        !keyRange.lowerKey.isNull() && !keyRange.lowerOpen ? ">="_s : ">"_s,
        " CAST(? AS TEXT) AND key "_s,
        !keyRange.upperKey.isNull() && !keyRange.upperOpen ? "<="_s : "<"_s,
        " CAST(? AS TEXT) ORDER BY key"_s,
        cursorDirection == IndexedDB::CursorDirection::Prev || cursorDirection == IndexedDB::CursorDirection::Prevunique ? " DESC"_s : ""_s,
        ", value"_s,
        cursorDirection == IndexedDB::CursorDirection::Prev ? " DESC"_s : ""_s,
        ';');
}

static String buildObjectStoreStatement(const IDBKeyRangeData& keyRange, IndexedDB::CursorDirection cursorDirection)
{
    return makeString("SELECT rowid, key, value FROM Records WHERE objectStoreID = ? AND key "_s,
        !keyRange.lowerKey.isNull() && !keyRange.lowerOpen ? ">="_s : ">"_s,
        " CAST(? AS TEXT) AND key "_s,
        !keyRange.upperKey.isNull() && !keyRange.upperOpen ? "<="_s : "<"_s,
        " CAST(? AS TEXT) ORDER BY key"_s,
        cursorDirection == IndexedDB::CursorDirection::Prev || cursorDirection == IndexedDB::CursorDirection::Prevunique ? " DESC"_s : ""_s,
        ';');
}

bool SQLiteIDBCursor::establishStatement()
{
    ASSERT(!m_statement);
    String sql;

    if (m_indexID) {
        sql = buildIndexStatement(m_keyRange, m_cursorDirection);
        m_boundID = *m_indexID;
    } else {
        sql = buildObjectStoreStatement(m_keyRange, m_cursorDirection);
        m_boundID = m_objectStoreID;
    }

    m_currentLowerKey = m_keyRange.lowerKey.isNull() ? IDBKeyData::minimum() : m_keyRange.lowerKey;
    m_currentUpperKey = m_keyRange.upperKey.isNull() ? IDBKeyData::maximum() : m_keyRange.upperKey;

    return createSQLiteStatement(sql);
}

bool SQLiteIDBCursor::createSQLiteStatement(StringView sql)
{
    LOG(IndexedDB, "Creating cursor with SQL query: \"%s\"", sql.utf8().data());

    ASSERT(!m_currentLowerKey.isNull());
    ASSERT(!m_currentUpperKey.isNull());
    ASSERT(m_transaction->sqliteDatabase());

    CheckedPtr database = m_transaction->sqliteDatabase();
    auto statement = database->prepareHeapStatementSlow(sql);
    if (!statement) {
        LOG_ERROR("Could not create cursor statement (prepare/id) - '%s'", database->lastErrorMsg());
        return false;
    }
    m_statement = statement.value().moveToUniquePtr();

    return bindArguments();
}

void SQLiteIDBCursor::objectStoreRecordsChanged()
{
    if (m_statementNeedsReset)
        return;

    ASSERT(!m_fetchedRecords.isEmpty());

    m_currentKeyForUniqueness = m_fetchedRecords.first().record.key;
    if (m_indexID)
        m_currentIndexRecordValue = m_fetchedRecords.first().record.primaryKey;

    // If ObjectStore or Index contents changed, we need to reset the statement and bind new parameters to it.
    // This is to pick up any changes that might exist.
    m_statementNeedsReset = true;

    if (isDirectionNext()) {
        m_currentLowerKey = m_currentKeyForUniqueness;
        if (!m_keyRange.lowerOpen) {
            m_keyRange.lowerOpen = true;
            m_keyRange.lowerKey = m_currentLowerKey;
            m_statement = nullptr;
        }
    } else {
        m_currentUpperKey = m_currentKeyForUniqueness;
        if (!m_keyRange.upperOpen) {
            m_keyRange.upperOpen = true;
            m_keyRange.upperKey = m_currentUpperKey;
            m_statement = nullptr;
        }
    }

    // We also need to throw away any fetched records as they may no longer be valid.
    m_fetchedRecords.clear();
    m_fetchedRecordsSize = 0;

    m_prefetchCount = 0;
}

void SQLiteIDBCursor::resetAndRebindStatement()
{
    ASSERT(!m_currentLowerKey.isNull());
    ASSERT(!m_currentUpperKey.isNull());
    ASSERT(m_transaction->sqliteDatabase());
    ASSERT(m_statementNeedsReset);

    m_statementNeedsReset = false;

    if (!m_statement && !establishStatement()) {
        LOG_ERROR("Unable to establish new statement for cursor iteration");
        return;
    }

    if (m_statement->reset() != SQLITE_OK) {
        LOG_ERROR("Could not reset cursor statement to respond to object store changes");
        return;
    }

    bindArguments();
}

bool SQLiteIDBCursor::bindArguments()
{
    LOG(IndexedDB, "Cursor is binding lower key '%s' and upper key '%s'", m_currentLowerKey.loggingString().utf8().data(), m_currentUpperKey.loggingString().utf8().data());

    int currentBindArgument = 1;

    if (m_statement->bindInt64(currentBindArgument++, boundIDValue()) != SQLITE_OK) {
        LOG_ERROR("Could not bind id argument (bound ID)");
        return false;
    }

    auto buffer = serializeIDBKeyData(m_currentLowerKey);
    if (m_statement->bindBlob(currentBindArgument++, buffer->span()) != SQLITE_OK) {
        LOG_ERROR("Could not create cursor statement (lower key)");
        return false;
    }

    buffer = serializeIDBKeyData(m_currentUpperKey);
    if (m_statement->bindBlob(currentBindArgument++, buffer->span()) != SQLITE_OK) {
        LOG_ERROR("Could not create cursor statement (upper key)");
        return false;
    }

    return true;
}

bool SQLiteIDBCursor::resetAndRebindPreIndexStatementIfNecessary()
{
    if (!m_indexID)
        return true;

    if (m_currentIndexRecordValue.isNull())
        return true;

    CheckedPtr database = m_transaction->sqliteDatabase();
    if (!m_preIndexStatement) {
        auto preIndexStatement = database->prepareHeapStatementSlow(buildPreIndexStatement(isDirectionNext()));
        if (!preIndexStatement) {
            LOG_ERROR("Could not prepare pre statement - '%s'", database->lastErrorMsg());
            return false;
        }
        m_preIndexStatement = preIndexStatement.value().moveToUniquePtr();
    }

    if (m_preIndexStatement->reset() != SQLITE_OK) {
        LOG_ERROR("Could not reset pre statement - '%s'", database->lastErrorMsg());
        return false;
    }

    auto key = isDirectionNext() ? m_currentLowerKey : m_currentUpperKey;
    int currentBindArgument = 1;

    if (m_preIndexStatement->bindInt64(currentBindArgument++, boundIDValue()) != SQLITE_OK) {
        LOG_ERROR("Could not bind id argument to pre statement (bound ID)");
        return false;
    }

    auto buffer = serializeIDBKeyData(key);
    if (m_preIndexStatement->bindBlob(currentBindArgument++, buffer->span()) != SQLITE_OK) {
        LOG_ERROR("Could not bind id argument to pre statement (key)");
        return false;
    }

    buffer = serializeIDBKeyData(m_currentIndexRecordValue);
    if (m_preIndexStatement->bindBlob(currentBindArgument++, buffer->span()) != SQLITE_OK) {
        LOG_ERROR("Could not bind id argument to pre statement (value)");
        return false;
    }

    return true;
}

bool SQLiteIDBCursor::prefetchOneRecord()
{
    LOG(IndexedDB, "SQLiteIDBCursor::prefetchOneRecord() - Cursor already has %zu fetched records", m_fetchedRecords.size());

    if (m_fetchedRecordsSize >= prefetchSizeLimit || m_fetchedRecords.isEmpty() || m_fetchedRecords.size() >= prefetchLimit || m_fetchedRecords.last().isTerminalRecord())
        return false;

    m_currentKeyForUniqueness = m_fetchedRecords.last().record.key;

    return fetch() && m_fetchedRecords.size() < prefetchLimit && m_fetchedRecordsSize < prefetchSizeLimit;
}

void SQLiteIDBCursor::increaseCountToPrefetch()
{
    m_prefetchCount = m_prefetchCount ? m_prefetchCount * 2 : 1;
}

bool SQLiteIDBCursor::prefetch()
{
    for (unsigned i = 0; i < m_prefetchCount; ++i) {
        if (!prefetchOneRecord())
            return false;
    }

    increaseCountToPrefetch();
    return true;
}

bool SQLiteIDBCursor::advance(uint64_t count)
{
    LOG(IndexedDB, "SQLiteIDBCursor::advance() - Count %" PRIu64 ", %zu fetched records", count, m_fetchedRecords.size());
    ASSERT(count);

    if (!m_fetchedRecords.isEmpty() && m_fetchedRecords.first().isTerminalRecord()) {
        LOG_ERROR("Attempt to advance a completed cursor");
        return false;
    }

    if (!m_fetchedRecords.isEmpty())
        m_currentKeyForUniqueness = m_fetchedRecords.last().record.key;

    // Drop already-fetched records up to `count` to see if we've already fetched the record we're looking for.
    bool hadCurrentRecord = !m_fetchedRecords.isEmpty();
    for (; count && !m_fetchedRecords.isEmpty(); --count) {
        if (m_fetchedRecords.first().isTerminalRecord())
            break;

        ASSERT(m_fetchedRecordsSize >= m_fetchedRecords.first().record.size());
        m_fetchedRecordsSize -= m_fetchedRecords.first().record.size();
        m_fetchedRecords.removeFirst();
    }

    // If we still have any records left, the first record is our new current record.
    if (!m_fetchedRecords.isEmpty())
        return true;

    ASSERT(m_fetchedRecords.isEmpty());

    // If we started out with a current record, we burnt a count on removing it.
    // Replace that count now.
    if (hadCurrentRecord)
        ++count;

    for (; count; --count) {
        if (!m_fetchedRecords.isEmpty()) {
            ASSERT(m_fetchedRecords.size() == 1);
            m_currentKeyForUniqueness = m_fetchedRecords.first().record.key;

            ASSERT(m_fetchedRecordsSize >= m_fetchedRecords.first().record.size());
            m_fetchedRecordsSize -= m_fetchedRecords.first().record.size();
            m_fetchedRecords.removeFirst();
        }

        if (!fetch())
            return false;

        ASSERT(!m_fetchedRecords.isEmpty());
        ASSERT(!m_fetchedRecords.first().errored);
        if (m_fetchedRecords.first().completed)
            break;
    }

    return true;
}

bool SQLiteIDBCursor::fetch()
{
    ASSERT(m_fetchedRecords.isEmpty() || !m_fetchedRecords.last().isTerminalRecord());

    m_fetchedRecords.append({ });

    bool isUnique = m_cursorDirection == IndexedDB::CursorDirection::Nextunique || m_cursorDirection == IndexedDB::CursorDirection::Prevunique;
    if (!isUnique) {
        bool fetchSucceeded = fetchNextRecord(m_fetchedRecords.last());
        if (fetchSucceeded)
            m_fetchedRecordsSize += m_fetchedRecords.last().record.size();
        return fetchSucceeded;
    }

    while (fetchNextRecord(m_fetchedRecords.last())) {
        m_fetchedRecordsSize += m_fetchedRecords.last().record.size();

        if (m_currentKeyForUniqueness != m_fetchedRecords.last().record.key)
            return true;

        if (m_fetchedRecords.last().completed)
            return false;

        m_fetchedRecordsSize -= m_fetchedRecords.last().record.size();
    }

    return false;
}

bool SQLiteIDBCursor::fetchNextRecord(SQLiteCursorRecord& record)
{
    if (m_statementNeedsReset) {
        resetAndRebindPreIndexStatementIfNecessary();
        resetAndRebindStatement();
    }

    FetchResult result;
    do {
        result = internalFetchNextRecord(record);
    } while (result == FetchResult::ShouldFetchAgain);

    return result == FetchResult::Success;
}

void SQLiteIDBCursor::markAsErrored(SQLiteCursorRecord& record)
{
    record.record = { };
    record.completed = true;
    record.errored = true;
    record.rowID = 0;
}

SQLiteIDBCursor::FetchResult SQLiteIDBCursor::internalFetchNextRecord(SQLiteCursorRecord& record)
{
    ASSERT(m_transaction->sqliteDatabase());
    ASSERT(m_statement);
    ASSERT(!m_fetchedRecords.isEmpty());
    ASSERT(!m_fetchedRecords.last().isTerminalRecord());

    record.record.value = { };

    CheckedPtr database = m_transaction->sqliteDatabase();
    SQLiteStatement* statement = nullptr;

    int result;
    if (m_preIndexStatement) {
        ASSERT(m_indexID);

        result = m_preIndexStatement->step();
        if (result == SQLITE_ROW)
            statement = m_preIndexStatement.get();
        else if (result != SQLITE_DONE)
            LOG_ERROR("Error advancing with pre statement - (%i) %s", result, database->lastErrorMsg());
    }
    
    if (!statement) {
        result = m_statement->step();
        if (result == SQLITE_DONE) {
            record = { };
            record.completed = true;
            return FetchResult::Success;
        }
        if (result != SQLITE_ROW) {
            LOG_ERROR("Error advancing cursor - (%i) %s", result, database->lastErrorMsg());
            markAsErrored(record);
            return FetchResult::Failure;
        }
        statement = m_statement.get();
    }

    record.rowID = statement->columnInt64(0);
    ASSERT(record.rowID);
    auto keyDataSpan = statement->columnBlobAsSpan(1);

    if (!deserializeIDBKeyData(keyDataSpan, record.record.key)) {
        LOG_ERROR("Unable to deserialize key data from database while advancing cursor");
        markAsErrored(record);
        return FetchResult::Failure;
    }

    auto keyData = statement->columnBlob(2);

    // The primaryKey of an ObjectStore cursor is the same as its key.
    if (!m_indexID) {
        record.record.primaryKey = record.record.key;

        Vector<String> blobURLs, blobFilePaths;
        auto error = m_transaction->backingStore().getBlobRecordsForObjectStoreRecord(record.rowID, blobURLs, blobFilePaths);
        if (!error.isNull()) {
            LOG_ERROR("Unable to fetch blob records from database while advancing cursor");
            markAsErrored(record);
            return FetchResult::Failure;
        }

        if (m_cursorType == IndexedDB::CursorType::KeyAndValue)
            record.record.value = { ThreadSafeDataBuffer::create(WTFMove(keyData)), blobURLs, blobFilePaths };
    } else {
        if (!deserializeIDBKeyData(keyData.span(), record.record.primaryKey)) {
            LOG_ERROR("Unable to deserialize value data from database while advancing index cursor");
            markAsErrored(record);
            return FetchResult::Failure;
        }

        if (!m_cachedObjectStoreStatement || m_cachedObjectStoreStatement->reset() != SQLITE_OK) {
            if (auto cachedObjectStoreStatement = database->prepareHeapStatement("SELECT rowid, value FROM Records WHERE key = CAST(? AS TEXT) and objectStoreID = ?;"_s))
                m_cachedObjectStoreStatement = cachedObjectStoreStatement.value().moveToUniquePtr();
        }

        if (!m_cachedObjectStoreStatement
            || m_cachedObjectStoreStatement->bindBlob(1, keyData) != SQLITE_OK
            || m_cachedObjectStoreStatement->bindInt64(2, m_objectStoreID.toRawValue()) != SQLITE_OK) {
            LOG_ERROR("Could not create index cursor statement into object store records (%i) '%s'", database->lastError(), database->lastErrorMsg());
            markAsErrored(record);
            return FetchResult::Failure;
        }

        int result = m_cachedObjectStoreStatement->step();

        if (result == SQLITE_ROW) {
            auto recordsRowID = m_cachedObjectStoreStatement->columnInt64(0);
            Vector<String> blobURLs, blobFilePaths;
            auto error = m_transaction->backingStore().getBlobRecordsForObjectStoreRecord(recordsRowID, blobURLs, blobFilePaths);
            if (!error.isNull()) {
                LOG_ERROR("Unable to fetch blob records from database while advancing cursor");
                markAsErrored(record);
                return FetchResult::Failure;
            }

            record.record.value = { ThreadSafeDataBuffer::create(m_cachedObjectStoreStatement->columnBlob(1)), WTFMove(blobURLs), WTFMove(blobFilePaths) };
        } else if (result == SQLITE_DONE) {
            // This indicates that the record we're trying to retrieve has been removed from the object store.
            // Skip over it.
            return FetchResult::ShouldFetchAgain;
        } else {
            LOG_ERROR("Could not step index cursor statement into object store records (%i) '%s'", database->lastError(), database->lastErrorMsg());
            markAsErrored(record);
            return FetchResult::Failure;
        }
    }

    return FetchResult::Success;
}

bool SQLiteIDBCursor::iterate(const IDBKeyData& targetKey, const IDBKeyData& targetPrimaryKey)
{
    ASSERT(m_transaction->sqliteDatabase());
    ASSERT(m_statement);

    bool result = advance(1);
    ASSERT(!m_fetchedRecords.isEmpty());

    // Iterating with no key is equivalent to advancing 1 step.
    if (targetKey.isNull() || !result)
        return result;

    while (!m_fetchedRecords.first().isTerminalRecord()) {
        if (!result)
            return false;

        // Search for the next key >= the target if the cursor is a Next cursor, or the next key <= if the cursor is a Previous cursor.
        if (m_cursorDirection == IndexedDB::CursorDirection::Next || m_cursorDirection == IndexedDB::CursorDirection::Nextunique) {
            if (m_fetchedRecords.first().record.key >= targetKey)
                break;
        } else if (m_fetchedRecords.first().record.key <= targetKey)
            break;

        result = advance(1);
    }

    if (targetPrimaryKey.isValid()) {
        while (!m_fetchedRecords.first().isTerminalRecord() && m_fetchedRecords.first().record.key == targetKey) {
            if (!result)
                return false;

            // Search for the next primary key >= the primary target if the cursor is a Next cursor, or the next key <= if the cursor is a Previous cursor.
            if (m_cursorDirection == IndexedDB::CursorDirection::Next || m_cursorDirection == IndexedDB::CursorDirection::Nextunique) {
                if (m_fetchedRecords.first().record.primaryKey >= targetPrimaryKey)
                    break;
            } else if (m_fetchedRecords.first().record.primaryKey <= targetPrimaryKey)
                break;

            result = advance(1);
        }
    }

    return result;
}

const IDBKeyData& SQLiteIDBCursor::currentKey() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().record.key;
}

const IDBKeyData& SQLiteIDBCursor::currentPrimaryKey() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().record.primaryKey;
}

const IDBValue& SQLiteIDBCursor::currentValue() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().record.value;
}

bool SQLiteIDBCursor::didComplete() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().completed;
}

bool SQLiteIDBCursor::didError() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().errored;
}

int64_t SQLiteIDBCursor::currentRecordRowID() const
{
    ASSERT(!m_fetchedRecords.isEmpty());
    return m_fetchedRecords.first().rowID;
}

uint64_t SQLiteIDBCursor::boundIDValue() const
{
    return std::holds_alternative<IDBObjectStoreIdentifier>(m_boundID) ? std::get<IDBObjectStoreIdentifier>(m_boundID).toRawValue() : std::get<IDBIndexIdentifier>(m_boundID).toRawValue();
}

} // namespace IDBServer
} // namespace WebCore
