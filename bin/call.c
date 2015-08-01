inherit CommandCode;

#include <expand_object.h>

#define DEFAULT_CONTEXT        "(here,me,users)"
#define CallFile(u)            (HomeDir + "/" + u + "/tmp/call.c")

private variables private functions inherit FileLib;
private variables private functions inherit ArgsLib;
private variables private functions inherit GetoptsLib;
private variables private functions inherit ObjectExpansionLib;

int do_command(string arg) {
  mixed *args;
  args = getopts(explode_args(arg), "");
  if (sizeof(args[0]) < 1) {
    notify_fail(sprintf("Usage: %s target func(args, ...)\n", query_verb()));
    return 0;
  }
  // FIXME figure out a way to preserve whitespace
  arg = implode(args[0][1..], " ");
  if (arg[<1] == ';') {
    arg = arg[0..<2];
  }

  string callfile = CallFile(UNAME);
  seteuid(getuid());
  object call_obj = FINDO(callfile);
  if (call_obj) {
    destruct(call_obj);
  }
  if (call_obj) {
    printf("%s: unable to destruct call object\n", query_verb());
    return 1;
  }

  if (file_exists(callfile)) {
    if (!rm(callfile)) {
      printf("%s: unable to rm call file\n", query_verb());
      return 1;
    }
  }

  if (strstr(arg, "(") == -1) {
    arg += "()";
  }

  write_file(callfile,
    "void create() { seteuid(getuid()); }\n"
    "mixed call(object ob) { return (ob->" + arg + "); }\n"
  );

  if (!(call_obj = load_object(callfile))) {
    printf("%s: unable to load call object\n", query_verb());
    return 1;
  }

  mixed *targets = expand_objects(args[0][0], THISP, DEFAULT_CONTEXT,
                                  MATCH_BLUEPRINTS|STALE_CLONES);
  int count = 0;
  string out = "";
  foreach (mixed *t : targets) {
    object target = t[OB_TARGET];
    mixed result;
    string err = catch (result = call_obj->call(target); publish);
    if (err) {
      printf("%s: %s: Caught error %s\n",
             query_verb(), object_name(target), err);
      return 1;
    }
    out += sprintf("%s: %s->%s\nresult: %O\n",
                   query_verb(), object_name(target), arg, result);
    count++;
  }

  out += sprintf("%s: %s: %d object%s called\n",
                 query_verb(), args[0][0], count, (count == 1 ? "" : "s"));
  write(out);
  return 1;
}
