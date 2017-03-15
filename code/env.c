#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "types.h"
#include "env.h"

/*Lab4: Your implementation of lab4*/
E_enventry E_VarEntry(Tr_access access, Ty_ty ty) {
    E_enventry e = checked_malloc(sizeof(*e));
    e->kind = E_varEntry; 
    e->u.var.access = access;
    e->u.var.ty = ty;
    return e;
}

E_enventry E_FunEntry(Tr_level level, Temp_label label, Ty_tyList formals, Ty_ty result) {
    E_enventry e = checked_malloc(sizeof(*e));
    e->kind = E_funEntry; 
    e->u.fun.formals = formals;
    e->u.fun.result = result;
    e->u.fun.level = level;
    e->u.fun.label = label;
    return e;
}

S_table E_base_tenv(void) {
    return S_empty();
}

S_table E_base_venv(void) {
    return S_empty();
}