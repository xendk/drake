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

function drake_argument($index, $description) {
  return new DrakeContextArgPlaceholder($index, $description);
}

/**
 * Implements the basic context placeholder object.
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

  /**
   * Resolves the placeholder to an value.
   */
  public function resolve(DrakeContext $context) {
    if (!preg_match_all('/\\[([^]]*)\\]/', $this->name, $matches, PREG_SET_ORDER)) {
      // Same structure as what preg_match_all returns.
      $matches = array(
        array(
          $this->name,
          $this->name,
        ),
      );
    }

    $values = array();
    foreach ($matches as $match) {
      $pipeline = explode(':', $match[1]);
      $value = $context[array_shift($pipeline)];
      foreach ($pipeline as $op) {
        $method = 'get' . ucfirst($op);
        if (is_object($value) && method_exists($value, $method)) {
          $value = $value->{$method}();
        }
        else {
          $class_name = "DrakeContextPlaceholder" . ucfirst($op) . "Op";
          if (class_exists($class_name)) {
            $value = new $class_name($value);
          }
          else {
            throw new DrakeContextException(dt('Unknown or invalid operation "@op" in context string "@context"', array('@op' => $op, '@context' => $match[1])));
          }
        }
      }
      $values[$match[0]] = (string) $value;
    }
    return strtr($this->name(), $values);
  }
}

class DrakeContextPlaceholderSiteOp {
  protected $site;

  public function __construct($value) {
    $this->site = $value;
  }
  public function __toString() {
    return $this->site;
  }

  public function getRoot() {
    $self_record = drush_sitealias_get_record($this->site);
    if (!empty($self_record)) {
      // Oddly, it might be '@self'.
      if ($self_record['#name'] == '@self') {
        $global_context['@self'] = '@self';
      }
      else {
        $global_context['@self'] = '@' . $self_record['#name'];
      }
      // Add in root.
      if (!empty($self_record['root'])) {
        return $self_record['root'];
      }
      throw new DrakeContextException(dt('No site root found for "@site"', array('@site' => $this->site)));
    }
    else {
      throw new DrakeContextException(dt('No "@site" site found', array('@site' => $this->site)));
    }
  }
}

class DrakeContextPlaceholderStringOp {
  protected $string;

  public function __construct($value) {
    $this->string = $value;
  }
  public function __toString() {
    return $this->string;
  }

  public function getUpcase() {
    return strtoupper($this->string);
  }

  public function getDowncase() {
    return strtolower($this->string);
  }
}

/**
 * Placeholder for arguments.
 */
class DrakeContextArgPlaceholder extends DrakeContextPlaceholder {
  protected $description;
  /**
   * Constructs the placeholder.
   */
  public function __construct($index, $description) {
    $this->name = $index;
    $this->description = $description;
  }

  /**
   * Resolves the placeholder to an value.
   */
  public function resolve(DrakeContext $context) {
    try {
      return $context['arg' . $this->name];
    }
    catch (DrakeContextNotFoundException $e) {
      throw new DrakeContextNotFoundException("Missing argument: " . $this->description);
    }
  }
}

/**
 * An context exception class.
 */
class DrakeContextException extends Exception {}
class DrakeContextNotFoundException extends DrakeContextException {}

/**
 * Implements the context object.
 *
 * A context wraps a set of variables for tasks. It may also have a parent
 * context, which is asked if the current context doesn't define a variable.
 */
class DrakeContext implements ArrayAccess {
  protected $parents;
  protected $values = array();

  /**
   * Constructs a new context.
   *
   * @param DrakeContext $parent
   *   Parent context.
   */
  public function __construct($parents = array()) {
    if (!is_array($parents)) {
      $parents = array($parents);
    }
    $this->parents = $parents;
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
    // Using while, as the value might also be a placeholder.
    while ($value instanceof DrakeContextPlaceholder) {
      $value = $value->resolve($this);
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
    foreach ($this->parents as $parent) {
      $return = array_merge($return, $parent->toArray());
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
    elseif (!empty($this->parents)) {
      foreach ($this->parents as $parent) {
        if ($parent->offsetExists($offset)) {
          return TRUE;
        }
      }
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
      if (!empty($this->parents)) {
        // Check backwards as latter values takes priority.
        foreach (array_reverse($this->parents) as $parent) {
          try {
            return $parent[$offset];
          }
          catch (DrakeContextNotFoundException $e) {
            // Do nothing, try the next parent.
          }
        }
      }
    }
    // Couldn't find a value, throw exception.
    throw new DrakeContextNotFoundException('No such context "' . $offset . '"');
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
