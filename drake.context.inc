<?php

/**
 * @file
 * Defines the DrakeContext object and helper functions which is used to handle
 * task context variables.
 */

function set_context($name, $value) {

}

function context ($name) {
  return new DrakeContextPlaceholder($name);
}

class DrakeContextPlaceholder {
  protected $name;

  public function __construct($name) {
    $this->name = $name;
  }

  public function name() {
    return $this->name;
  }
}

class DrakeContext implements ArrayAccess {
  protected $parent;
  protected $values = array();

  public function __construct($parent = NULL) {
    $this->parent = $parent;
  }

  function resolve(&$array) {
    array_walk_recursive($array, array($this, 'resolve_value'));
  }

  function resolve_value(&$value) {
    if ($value instanceof DrakeContextPlaceholder) {
      $value = $this[$value->name()];
    }
  }

  /**
   * Implements ArrayAccess::offsetExists().
   */
  public function offsetExists($offset) {
    if (isset($this->values[$offset]) || array_key_exists($offset, $this->values)) {
      return TRUE;
    }
    elseif ($this->parent) {
      return $this->parent->offsetExists($offset);
    }
    return FALSE;
  }

  /**
   * Implements ArrayAccess::offsetGet().
   */
  public function offsetGet($offset) {
    if (isset($this->values[$offset]) || array_key_exists($offset, $this->values)) {
      return $this->values[$offset];
    }
    else {
      if ($this->parent) {
        return $this->parent[$offset];
      }
      else {
        // @todo Dedicated exception, or something else?
        throw new Exception('No such context ' . $offset);
      }
    }
  }

  /**
   * Implements ArrayAccess::offsetSet().
   */
  public function offsetSet($offset, $value) {
    $this->values[$offset] = $value;
  }

  /**
   * Implements ArrayAccess::offsetUnset().
   */
  public function offsetUnset($offset) {
    unset($this->values[$offset]);
  }

}
