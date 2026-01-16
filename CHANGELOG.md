# CHANGELOG

### v0.3.1 (???)

- Bug-fix: Unqueued rerun request are now validated as well.
- Bug-fix: Implemented invalid state acknoledgment system, to avoid this
situation:
    1. Client sends 2 events in a row. The second is invalid, given the first
    one. Invalid state is sent from server to client.
    2. Before receiving the invalid state message, the client sends a third
    event, which is valid. Then restores old state and unfades.
    3. The server processes the new event and returns the newer state.
    4. An update then happens in the client, which is unnexpected, because the
    return from transparency should mean that all events have been processed,
    and there are no other ones in queue.
- Cache-busting: Introduced cache-busting system. Generated HTML pages now use
cache-bustable links to resources that are likely to change from version to
version of Lit.
- New logo.

### v0.3.0 (2026-01-11)

- Breaking-change: Lit is distributed with its own docs, but not with its built
docs, so in order for it to be served, the user has to specify a path where it
has been built. So we added the ability to do so. Now `start_path()`, instead of
accepting a `docs::Bool`, accepts a `docs_path::String`.
- Bug-fix: Invalid URIs used to return HTTP status 200 with empty HTML. That
has been fixed to return 404 and a proper 404.html page.
- Bug-fix: To-Do now handles `nothing` as a possible value returned by the
text box. [Issue #4](https://github.com/nidoro/Lit.jl/issues/4)

### v0.2.4 (2026-01-10)

- Hot-fix: solved liblit.so compatibility issues with older linux versions.

### v0.2.3 (2026-01-10)

- Added Windows 64-bit support.
- Brazil's forecast example now downloads the shape file if it doesn't exist.

### v0.2.2 (2026-01-06)

- Bug-fix: rerun requests are now serialized in the back and front-ends.
- Bug-fix: implemented proper paste behaviour for `dataframe`.

