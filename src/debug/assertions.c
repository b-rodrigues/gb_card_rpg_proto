#include "assertions.h"

/* ROM-side assertion helpers were removed: the protocol contract (§41) makes
 * the host-side test runner the authoritative assertion evaluator, and these
 * functions had no ROM callers.  Assertions live in tools/test_runner.py. */
