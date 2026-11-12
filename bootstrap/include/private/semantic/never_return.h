/* Declares the never-returning call query (docs/en/reference/errors.md, "Never returning"). */

#ifndef SULTANC__SEMANTIC_NEVER_RETURN_H__
#define SULTANC__SEMANTIC_NEVER_RETURN_H__

#include "semantic/context.h"

/* Answers whether EXPRESSION is a direct call of a function that never
 * returns: إنهيار of the trusted std module process/panic.sn. Mirrors
 * استخلص_رموز_لا_تعود_واجهة (compiler/frontend/semantic_pipeline.sn): the
 * module is identified by its canonical path under the project's std root,
 * never by a spelling a user module could copy. The caller has already
 * excluded a local of the same name. */
int __Semantic_Call_Never_Returns__(__Semantic_Context__ *__Context__,
                                    const __Program_Unit__ *__From_Unit__,
                                    const __Ast_Expression__ *__Expression__);

#endif
