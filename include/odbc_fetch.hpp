#ifndef ODBC_FETCH_HPP
#define ODBC_FETCH_HPP

#include "odbc_common.hpp"

#include <sqltypes.h>
#include <sqlext.h>


namespace duckdb_odbc {

struct OdbcHandleStmt;

class OdbcFetch {
public:
	const static SQLULEN SINGLE_VALUE_FETCH = 1;
	const static SQLRETURN RETURN_FETCH_BEFORE_START = 999;

	SQLULEN cursor_type;
	SQLULEN cursor_scrollable;
	SQLLEN row_count;

	struct {
		row_t col_idx;
		row_t row_idx;
		size_t length;
	} last_fetched_variable_val;

private:
	OdbcHandleStmt *hstmt_ref;
	// main structure to hold the fetched chunks
	vector<duckdb_odbc::unique_ptr<DataChunk>> chunks;
	// the rows of the result, read from DuckDB when the statement is executed, see Prefetch
	duckdb_odbc::unique_ptr<cxx::ColumnDataCollection> collection;
	duckdb_odbc::unique_ptr<cxx::ColumnDataCollection::AppendState> append_state;
	duckdb_odbc::unique_ptr<cxx::ColumnDataCollection::SharedScanState> shared_scan;
	duckdb_odbc::unique_ptr<cxx::ColumnDataCollection::WorkerScanState> worker_scan;
	vector<LogicalType> collection_types;
	// the result was read to its end by Prefetch, but is kept for its metadata until the next read releases it
	bool result_exhausted = false;
	// used by fetch prior
	duckdb_odbc::idx_t current_chunk_idx;
	duckdb_odbc::DataChunk *current_chunk;
	row_t chunk_row;
	row_t prior_chunk_row;

	// flag the end of the result set has reached
	// it's important because ODBC can reuse the result set many times
	bool resultset_end;

public:
	explicit OdbcFetch(OdbcHandleStmt *hstmt)
	    : cursor_type(SQL_CURSOR_FORWARD_ONLY), cursor_scrollable(SQL_NONSCROLLABLE), row_count(0), hstmt_ref(hstmt),
	      resultset_end(false) {
		ResetLastFetchedVariableVal();
	}
	~OdbcFetch();

	inline void AssertCurrentChunk() {
		D_ASSERT(chunk_row <= ((row_t)current_chunk->GetRowCount()));
	}

	SQLRETURN Fetch(OdbcHandleStmt *hstmt, SQLULEN fetch_orientation, SQLLEN fetch_offset);

	SQLRETURN FetchFirst(OdbcHandleStmt *hstmt);

	SQLRETURN FetchNextChunk(SQLULEN fetch_orientation, OdbcHandleStmt *hstmt, SQLLEN fetch_offset);

	SQLRETURN DummyFetch();

	//! The value of a column in the current row. Requires a current chunk.
	Value GetValue(SQLUSMALLINT col_idx);
	bool HasCurrentChunk() const {
		return current_chunk != nullptr;
	}

	void ClearChunks();

	//! Releases the statement's result, which Prefetch read in full, so that the connection can run another
	//! statement while this one is still being fetched from.
	SQLRETURN Materialize(OdbcHandleStmt *hstmt);

	//! Reads the result of a statement that was just executed into memory, as the chunks of a collection that are
	//! then fetched from: DuckDB runs a statement as its result is read, so this is what executes it, and reports its
	//! errors. The rows come in chunks of whatever size the query produced them in; the collection packs them into
	//! full chunks, which fetching relies on to fill whole rowsets. The result is kept so that its metadata can still
	//! be read; ReleaseIfExhausted then releases it.
	SQLRETURN Prefetch(OdbcHandleStmt *hstmt);
	void ReleaseIfExhausted(OdbcHandleStmt *hstmt);

	void ResetLastFetchedVariableVal();
	void SetLastFetchedVariableVal(row_t col_idx);
	void SetLastFetchedLength(size_t new_len);
	size_t GetLastFetchedLength();

	bool IsInExecutionState();

	SQLLEN GetRowCount();

private:
	SQLRETURN ColumnWise(OdbcHandleStmt *hstmt);

	SQLRETURN RowWise(OdbcHandleStmt *hstmt);

	inline bool RequireFetch() {
		return (chunks.empty() || (chunk_row >= ((duckdb_odbc::row_t)chunks.back()->GetRowCount()) - 1));
	}

	void IncreaseRowCount();

	SQLRETURN FetchNext(OdbcHandleStmt *hstmt);

	//! The next chunk of the result; null at its end. Throws on error.
	duckdb_odbc::unique_ptr<DataChunk> ReadChunk(OdbcHandleStmt *hstmt);

	SQLRETURN ReportError(OdbcHandleStmt *hstmt, const std::exception &ex);

	SQLRETURN SetCurrentChunk(OdbcHandleStmt *hstmt);

	SQLRETURN SetPriorCurrentChunk(OdbcHandleStmt *hstmt);

	SQLRETURN BeforeStart();

	SQLRETURN SetAbsoluteCurrentChunk(OdbcHandleStmt *hstmt, SQLLEN fetch_offset);

	SQLRETURN SetFirstCurrentChunk(OdbcHandleStmt *hstmt);

	void SetRowStatus(idx_t row_idx, SQLINTEGER status);
};
} // namespace duckdb_odbc

#endif // ODBC_FETCH_HPP
