inherit CommandCode;

int do_command(string arg) {
  if (!arg || !strlen(arg)) {
    notify_fail("Usage: %s file\n", query_verb());
    return 0;
  }

  // TODO add path expansion
  if (is_directory(arg)) {
    printf("%s: %s: can only touch regular files.\n", query_verb(), arg);
    return 1;
  }

  if (!write_file(arg, "")) {
    printf("%s: %s: Unable to touch file.\n", query_verb(), arg);
    return 1;
  }

  return 1;
}
