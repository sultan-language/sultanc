/* Declares the bootstrap-native Stage1 executable finalizer. */

#ifndef SULTANC_BOOTSTRAP_LINK_STAGE1_FINALIZER_H
#define SULTANC_BOOTSTRAP_LINK_STAGE1_FINALIZER_H

int __Bootstrap_Finalize_Stage1__(const char *object_path,
                                  const char *output_path,
                                  const char *target_name);
const char *__Bootstrap_Stage1_Finalizer_Error__(void);

#endif
