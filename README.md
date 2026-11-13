## Running the ODBC Client and Tests

#### How the driver uses DuckDB

The driver is written against DuckDB's stable C++ API (`duckdb::cxx`), which wraps the DuckDB C API. Only that API is
vendored, in `third_party/duckdb_cpp`, together with the DuckDB revision it was taken from
(`third_party/duckdb_cpp/duckdb_revision.cmake`). The DuckDB engine of that revision is built separately, with the
`core_functions`, `parquet`, `icu` and `json` extensions, and statically linked into the driver, so the driver is still
a single self-contained library.

`make` builds the engine on first use, into `build/duckdb`, through `scripts/build_duckdb.py`. To link an engine you
built yourself, pass its install prefix instead:

```bash
python3 scripts/build_duckdb.py --install-dir /path/to/duckdb-install --source-dir /path/to/duckdb
make DUCKDB_ROOT=/path/to/duckdb-install
```

To move the driver to another DuckDB revision, vendor its C++ API from a DuckDB checkout:

```bash
python3 vendor.py --duckdb /path/to/duckdb
```

#### Build the ODBC client

###### Debug (for development)

```bash
make debug
```
###### Release (for usage)

```bash
make
```

#### Run the ODBC Unit Tests

The ODBC tests are written with the catch framework. To run the tests, run the following command from the main DuckDB repository:

```bash
build/debug/test/test_odbc
```

You can also individually run the tests by specifying the test name as an argument to the test executable:

```bash
build/debug/test/test_odbc 'Test ALTER TABLE statement'
```