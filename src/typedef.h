/*  Copyright: Christian Feldmann (christian.feldmann@bitmovin.com)
*/

// Convenience macro definitions which can be used in if clauses:
// if (is_Q_OS_MAC) ...
#ifdef Q_OS_MAC
const bool is_Q_OS_MAC = true;
#else
const bool is_Q_OS_MAC = false;
#endif