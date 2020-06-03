#ifndef UIDTASK_H
#define UIDTASK_H

#include <linux/hashtable.h>

#define UIDTASK_BITS 10
DECLARE_HASHTABLE(extern UIDTASK_LOOKUP, UIDTASK_BITS);

// Maps certain UID onto numeric value.
struct uidtask {
  uid_t uid;
  u64 acc;
  struct hlist_node node;
};

struct uidtask_lookup_conf {
  // Called for each task and coresponding uidtask structure.
  long (*present)(struct uidtask*, struct task_struct*);
  // When finished collecting data from all tasks.
  long (*fold)(uid_t*);
};

long uidtask_lookup(const struct uidtask_lookup_conf *, uid_t *);

#endif /* UIDTASK_H */
