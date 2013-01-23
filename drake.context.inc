<?php

/**
 * @file
 * Defines the DrakeContext object and helper functions which is used to handle
 * task context variables.
 */

/**
 * Returns a context placeholder.
 *
 * Task definitions use this to signal that a given parameter should be taken
 * from the context. It is then later replaced by the actual value before
 * invoking the action.
 */
function context($name) {
  return new DrakeContextPlaceholder($name);
}

/**
 * Implements the context placeholder object.
 */
class DrakeContextPlaceholder {
  protected $name;

  /**
   * Constructs the placeholder.
   */
  public function __construct($name) {
    $this->name = $name;
  }

  /**
   * Returns the name of this placeholder.
   */
  public function name() {
    return $this->name;
  }
}

/**
 * An context exception class.
 */
class DrakeContextException extends Exception {}

/**
 * Implements the context object.
 *
 * A context wraps a set of variables for tasks. It may also have a parent
 * context, which is asked if the current context doesn't define a variable.
 */
class DrakeContext implements ArrayAccess {
  protected $parent;
  protected $values = array();

  /**
   * Constructs a new context.
   *
   * @param DrakeContext $parent
   *   Parent context.
   */
  public function __construct(DrakeContext $parent = NULL) {
    $this->parent = $parent;
  }

  /**
   * Resolves context placeholders to actual values.
   *
   * Recursively replaces the DrakeContextPlaceholder objects in the provided
   * array, with values from this context.
   *
   * @param array $array
   *   The array to replace placeholders in.
   */
  function resolve(&$array) {
    array_walk_recursive($array, array($this, 'resolve_value'));
  }

  /**
   * Possibly resolve a single context value.
   *
   * Helper method for DrakeContext::resolve().
   * If the given value is not a context placeholder, nothing is done.
   */
  function resolve_value(&$value) {
    if ($value instanceof DrakeContextPlaceholder) {
      $value = $this[$value->name()];
    }
  }

  /**
   * Take an MD5 sum of this context.
   *
   * Gives a unique id for this context, depending on content.
   */
  function md5() {
    return md5(serialize($this->toArray()));
  }

  /**
   * Get the context as an array.
   *
   * Gets the context content from this context and its parents as a simple
   * array.
   */
  function toArray() {
    $return = array();
    if ($this->parent) {
      $return = $this->parent->toArray();
    }
    return array_merge($return, $this->values);
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
        throw new DrakeContextException('No such context "' . $offset . '"');
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
