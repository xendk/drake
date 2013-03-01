<?php

/**
 * @file
 * File handling helper functionality.
 */

function files($root, $includes = array(), $excludes = array()) {
  return new DrakeFilesPlaceholder($root, $includes, $excludes);
}

class DrakeFile {
  protected $root;
  protected $file_path;

  function __construct(DrakeFilesPlaceholder $root, $path) {
    $this->root = $root;
    $this->file_path = $path;
  }

  function __toString() {
    return $this->root->root() . '/' . $this->file_path;
  }

  function path() {
    return $this->file_path;
  }
}

class DrakeFilesPlaceholder implements DrakePlaceholder {
  protected $root;
  protected $includes;
  protected $excludes;

  function __construct($root, $includes, $excludes) {
    $this->root = rtrim($root, '/');
    $this->includes = $includes;
    $this->excludes = $excludes;
  }

  function root() {
    return $this->root;
  }

  function resolve(DrakeContext $context) {
    $includes = $this->pregify($this->includes);
    $excludes = $this->pregify($this->excludes);

    // @todo This is a ineffective way to do it, but this is proof of concept of
    // the file matching functionality. We should do our own scanning.
    $files = array_keys(drush_scan_directory($this->root, '/.*/'));

    $matched_files = array();
    $length = strlen($this->root) + 1;
    foreach ($files as $file) {
      $file_path = substr($file, $length);
      // $file_path = $file;
      foreach ($includes as $include) {
        if (preg_match($include, $file_path)) {
          // File is included, check if it's excluded.
          foreach ($excludes as $exclude) {
            if (preg_match($exclude, $file_path)) {
              // Break out two loops.
              continue 2;
            }
          }
          $matched_files[] = new DrakeFile($this, $file_path);
        }
      }
    }
    return $matched_files;
  }

  function pregify($patterns) {
    // RegExp escaping: an extreme sport..
    $from = array(
      '/\\\\\\?/',
      '/\\\\\\*\\\\\\*/',
      '/\\\\\\*/',
    );
    $to = array(
      '.',
      '.*',
      '[^/]*',
    );

    $preg_patterns = array();
    if (!is_array($patterns)) {
      $patterns = array($patterns);
    }
    foreach ($patterns as $pattern) {
      $pattern = preg_replace($from, $to, preg_quote($pattern, '{}'));
      // Anchor pattern to start of string, if it starts with a slash.
      if ($pattern[0] == '/') {
        $pattern[0] = '^';
      }
      // Anchor to end.
      $pattern .= '$';
      $preg_patterns[] = '{' . $pattern . '}';
    }

    return $preg_patterns;
  }
}
