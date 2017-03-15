typedef struct Esc_binding_ *Esc_binding;

struct Esc_binding_{
  int depth;
  bool* escape;
};

Esc_binding Esc_newBinding(int depth,bool* escape);
void Esc_findEscape(A_exp exp);
