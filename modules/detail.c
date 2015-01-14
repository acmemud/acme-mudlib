/**
 * The detail mixin is for rooms, creatures, and items which wish to add
 * extra ids which the user may look at to read a more detailed description
 * of that part of the object. The detail map is a hierarchy, with ids
 * separated by periods '.'.
 *
 * <p>For instance, a room description may make mention of a painting in the
 * room. The user may then "look painting" to get a more detailed
 * description. If the painting description then refers to something in the
 * picture, like a hat, you may also "look painting.hat" to get a more
 * detailed description of the hat.</p>
 *
 * @alias DetailMixin
 */

#include <capabilities.h>

// FUTURE configure mixin to automatically highlight valid ids inside descs

#define PATH_DELIM "."

private variables private functions inherit ArrayLib;

default private variables;

/** intern the descriptions. even though the driver does this for us, creating
    our own string table will reduce the footprint of state files */
string *descriptions;

/** ([ str id : int desc_index; ([ details ]) ]) */
mapping details;

default public functions;

private mapping get_parent_details(string *path);
private varargs mapping resolve_parent(string parent, string id);
private string *resolve_child_ids(mapping det, string parent);
private int add_description(string description);

/**
 * Get the detail description for the specified id relative to the specified
 * parent detail id. If parent id is unspecified, the root ids will be
 * checked.
 *
 * @param  id     the id of the description
 * @param  parent optional parent id
 * @return        the detail description, or 0 if the id was not found
 */
varargs string query_detail(string id, string parent) {
  mapping det = resolve_parent(parent, &id);
  if (!det) {
    return 0;
  }
  if (!member(det, id)) {
    return 0;
  }
  return descriptions[det[id, 0]];
}

/**
 * Get all the detail ids that have been assigned descriptions, relative to
 * the specified parent detail id. If parent id is unspecified, the root ids
 * will be returned.
 *
 * @param  parent optional parent id
 * @return        an array of all detail ids
 */
varargs string *query_detail_ids(string parent) {
  // resolve parent if specified
  mapping det = resolve_parent(parent);
  if (!det) {
    return 0;
  }
  return m_indices(det);
}

/**
 * Return an array of the paths of all valid detail ids, starting from the
 * optional parent id, in depth-first order.
 *
 * @return the array of detail id paths, relative to parent
 */
varargs string *query_deep_detail_ids(string parent) {
  // resolve parent if specified
  mapping det = resolve_parent(parent);
  if (!det) {
    return 0;
  }
  return resolve_child_ids(det, "");
}

/**
 * Set a new detail description for one or more ids.
 *
 * @param  ids         a string or array of strings which are the new ids to
 *                     set
 * @param  description the description for the new detail ids
 * @param  parent      an optional parent id to which the new ids will be
 *                     relative
 * @return             the number of ids set
 */
varargs int set_detail(mixed ids, string description, string parent) {
  int result;

  // take a string or array of strings
  if (!pointerp(ids)) {
    ids = ({ ids });
  }

  // now add our ids
  foreach (string id : ids) {
    // resolve parent if specified
    mapping det = resolve_parent(parent, &id);
    if (!det) {
      continue;
    }

    // we're going to use this same mapping reference for all ids
    mapping children;

    int desc_index = member(descriptions, description);
    if (desc_index == -1) {
      // new description, new child map
      desc_index = add_description(description);
      children = m_allocate(0, 2);
    } else {
      // find the first matching detail description and share its children
      foreach (string key, int di, mapping c : det) {
        if (desc_index == di) {
          children = c;
          break;
        }
      }
    }

    det += ([ id : desc_index; children ]);
    result++;
  }
  return result;
}

/**
 * Remove one or more detail ids.
 *
 * @param  ids    a string or list of strings that is the ids to be removed
 * @param  parent an optional parent id which the specified ids are relative
 *                to
 * @return        the number of ids removed
 */
varargs int remove_detail(mixed ids, string parent) {
  int result;

  // take a string or array of strings
  if (!pointerp(ids)) {
    ids = ({ ids });
  }

  mapping desc_indexes = ([ ]);
  foreach (string id : ids) {
    // resolve parent if specified
    mapping det = resolve_parent(parent, &id);
    if (!det) {
      continue;
    }

    desc_indexes += ([ det[id, 0] ]);
    m_delete(det, id);
    result++;
  }

  // clean up any orphaned descriptions
  /* FIXME this can null out descriptions that are shared by non-sibling
     detail ids, we probably need a deep search from the root
  foreach (int i : desc_indexes) {
    if (!sizeof(filter(det, (: $2 == $4 :), i))) {
      descriptions[i] = 0;
    }
  }
  */

  return result;
}

/**
 * For a given id path, return the parent detail map containing the
 * description for that id and any nested detail ids.
 *
 * @param  path the array of path components derived from an absolute id
 * @return      the detail mapping
 */
private mapping get_parent_details(string *path) {
  mapping map = details;
  for (int i = 0, int j = sizeof(path) - 1; i < j; i++) {
    if (!member(map, path[i])) {
      return 0;
    }
    map = map[path[i], 1];
  }
  return map;
}

/**
 * Find our working detail map for a given parent. If the parent is
 * unspecified, the top-level detail map will be returned.
 *
 * @param  parent an optional parent id to resolve
 * @param  id     an optional relative id which also contain parent specifiers
 *                if passed by reference, will be truncated to just child id
 *                (e.g. resolve_parent("a", "b.c") returns "a.b" map and
 *                sets id to "c")
 * @return        the detail map for that id
 */
private varargs mapping resolve_parent(string parent, string id) {
  mapping det;
  parent ||= "";

  // resolve path delimiters in the id
  if (id) {
    int pos = searcha(id, PATH_DELIM[0], strlen(id) - 1, -1);
    if (pos != -1) {
      if (stringp(parent) && strlen(parent)) {
        parent = parent + PATH_DELIM + id[0..(pos - 1)];
      } else {
        parent = id[0..(pos - 1)];
      }
      id = id[(pos + 1)..];
    }
  }

  if (!strlen(parent)) {
    det = details;
  } else {
    string *path = explode(parent, PATH_DELIM);
    det = get_parent_details(path);
    if (!det) {
      return 0;
    }
    det = det[path[<1], 1];
  }
  return det;
}

/**
 * Recursive function to find all the nested ids of the detail map. A parent
 * path must be specified, which is used for resolving relatively id paths.
 *
 * @param  det    the detail mapping to search
 * @param  parent the parent id of the searched mapping
 * @return        an flattened array of all nested child ids
 */
private string *resolve_child_ids(mapping det, string parent) {
  string *result = ({ });
  foreach (string id, string desc, mapping children : det) {
    result += ({ id });
    string path = sprintf("%s%s%s",
                          parent, (strlen(parent) ? PATH_DELIM : ""), id);
    result += resolve_child_ids(children, path);
  }
  return result;
}

/**
 * Add a new description to the description table. The string will be
 * assigned to the first empty slot, or a new element will be added to the
 * end of the descriptions array.
 *
 * @param  description the new description to add
 * @return             the index of the added description
 */
private int add_description(string description) {
  int i;
  i = member(descriptions, 0);
  if (i != -1) {
    descriptions[i] = description;
    return i;
  } else {
    descriptions += ({ description });
    return sizeof(descriptions) - 1;
  }
}

/**
 * Initialize DetailMixin.
 */
protected void setup_detail() {
  descriptions = ({ });
  details = m_allocate(0, 2);
}

/**
 * Return the capabilities this mixin provides.
 *
 * @return the 'detail' capability
 */
public mapping query_capabilities() {
  return ([ CAP_DETAIL ]);
}
