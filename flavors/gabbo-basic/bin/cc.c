#include <sys/commands.h>

#define CcDir sprintf("%s/%s/tmp", HomeDir, UNAME)
#define USAGE "cc<id> [-msdrf] [-h path] [-i path] code"

inherit CommandCode;

private variables private functions inherit ArgsLib;
private variables private functions inherit GetoptsLib;
private variables private functions inherit FileLib;

mixed *query_actions() {
  return ({ ({ "cc", AA_SHORT }) });
}

string format_cc(string str);

int do_command(string arg) {
  // TODO design a formal usage API
  if (!arg || !strlen(arg)) { 
    notify_fail("Usage: " USAGE "\n"); 
    return 0; 
  }

  string label = query_verb(0);
  mixed *args = getopts(explode_args(arg, 1), "msdrfoh:i:");

  arg = implode(args[0], " ");
  mapping opts = args[1];

  string cc = sprintf("%s%s%s\n%s", 
    (!member(opts, 'o') ? "mixed cc() {\n" : ""),
    arg,
    ((arg[<1] != ';') ? ";" : ""),
    (!member(opts, 'o') ? "}\n" : "")
  );
  if (member(opts, 'f')) {
    cc = format_cc(cc);
  }
  
  string path = sprintf("%s/%s.c", CcDir, label);

  object cc_ob = FINDO(path);
  if (cc_ob) { 
    destruct(cc_ob);
    if (cc_ob) {
      printf("%s: %O: Unable to destruct existing cc object.\n", 
        query_verb(), cc_ob);
      return 1;
    }
  }
  
  if (file_exists(path)) {
    if (!rm(path)) {
      printf("%s: %s: Unable to remove existing cc file.\n", 
        query_verb(), path);
      return 1;
    }
  }

  if (member(opts, 'h')) {
    if (file_exists(opts['h'])) {
      if (copy_file(opts['h'], path)) {
        printf("%s: %s: Unable to apply template %s", 
          query_verb(), path, opts['h']);
        return 1;
      }
    }
  }
  if (!write_file(path, cc)) {
    printf("%s: %s: Unable to write new cc file.\n", query_verb(), path);
    return 1;
  }

  string err = catch (cc_ob = load_object(path); publish);
  if (err) {
    printf( "%s: %s\n", query_verb(), err);
    mixed *last_err = get_error_file(MasterObject->get_wiz_name(path));
    if (last_err) {
      printf("%s line %d: %s\n", last_err[0], last_err[1], last_err[2]);
    }
    printf( "Check your error log for details.\n" );
    return 1;
  }

  mixed result;
  err = catch (result = cc_ob->cc());
  if (err) {
    printf("%s: %s\n", query_verb(), err);
    return 1;
  }

  if (member(opts, 'd')) {
    destruct(cc_ob);
    if (cc_ob) {
      printf("%s: %O: Unable to destruct new cc object.\n",  
        query_verb(), cc_ob);
    }
  } 

  if (member(opts, 'r')) {
    if (!rm(path)) {
      printf("%s: %s: Unable to remove new cc file.\n", query_verb(), path);
    }
  }

  if (!member(opts, 's')) {
    if (member(opts, 'm')) {
      // TODO add more_string support
      printf("Returned: %O\n", result);
    } else {
      printf("Returned: %O\n", result);
    }
  }
   
  return 1;
}

string format_cc(string str) {
  int odd;
  for (int i = 0, int len = strlen(str); i < len; i++) {
    switch (str[i]){

      case '\'':
        if (!odd && str[i-1] != '#') {
          odd = '\'';
        } else if (odd == '\'') {
          odd = 0;
        }
        break;

      case '\"':  // "
        if (!odd) {
          odd = '\"';  // "
        } else if (odd == '\"') { // "
          odd = 0; 
        }
        break;

      case '\\':
        i++;
        break;

      case '\;':
        if (!odd) {
          str[i..i] = sprintf("%c\n", str[i]);
          len++;
        }
        break;
    }
  }
  return str;
}
