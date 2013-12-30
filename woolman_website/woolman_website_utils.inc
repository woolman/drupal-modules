<?php

/**
 * Output a list of options in the number of columns specified by the element's
 * #columns value.
 */
function theme_multicolumn_options($element) {
  // Initialize variables.
  $total_columns = $element['#columns'];
  $total_options = count($element['#options']);
  $options_per_column = ceil($total_options / $total_columns);
  $keys = array_keys($element['#options']);

  // Start wrapper div.
  $output = '<div class="multicolumn-options-wrapper">';
  $current_column = 1;
  $current_option = 0;

  while ($current_column <= $total_columns) {
    // Start column div.
    $output .=  '<div class="multicolumn-options-column" style="width: ' . 100 / $total_columns . '%; float: left">';

    // Keep looping through until the maximum options per column are reached,
    // or you run out of options.
    while ($current_option < $options_per_column * $current_column &&
           $current_option < $total_options) {

      // Output as either check or radio button depending on the element type.
      $output .= theme($element[$keys[$current_option]]['#type'], $element[$keys[$current_option]]);
      $current_option++;
    }

    // End column div.
    $output .= '</div>';
    $current_column++;
  }

  // End wrapper div.
  $output .= '</div><div class="clear-block"></div>';

  return $output;
}

/**
 * Autocomplete callback
 */
function woolman_website_autocomplete($type, $str= '') {
  $matches = array();
  if ($str) {
    $sub = '';
    switch($type) {

      case 'contact': case 'Organization': case 'Individual': case 'hs':
        if ($type == 'hs') {
          $type = 'Organization';
          $sub = "AND con.contact_sub_type = 'High_School'";
        }
        $db = "SELECT con.id, con.display_name, con.sort_name, ad.city, st.abbreviation AS state
                FROM {civicrm_contact} con
                LEFT JOIN {civicrm_address} ad ON con.id = ad.contact_id AND ad.is_primary = 1
                LEFT JOIN {civicrm_state_province} st ON st.id = ad.state_province_id
                WHERE (con.display_name LIKE '%%%s%%' OR con.sort_name LIKE '%%%s%%') AND con.is_deleted <> 1";
        $arg = array($str, $str);
        if ($type != 'contact') {
          $db .= " AND con.contact_type = '%s' $sub";
          $arg[] = $type;
        }
        $db .= " ORDER BY con.sort_name
                LIMIT 30";
        $resource = db_query($db, $arg);
        while ($row = db_fetch_array($resource)) {
          if ($row['city'] || $row['state']) {
            $addr = ': ' . $row['city'].($row['city'] ? ', ' : '').$row['state'];
          }
          else {
            $addr = '';
          }
          $matches[$row['display_name'].' (ID#' . $row['id'].')'] = $row['sort_name'].$addr;
        }
        break;

      case 'state':
        $resource = db_query("SELECT name, abbreviation FROM {civicrm_state_province} WHERE abbreviation LIKE '%s%%' AND country_id = 1228 LIMIT 30", $str);
        while ($row = db_fetch_array($resource)) {
          $matches[$row['abbreviation']] = $row['abbreviation'].' ' . $row['name'];
        }
        break;

      case 'email':
        $items = explode(',', $str);
        foreach($items as &$i) {
          $i = trim($i);
        }
        $str = array_pop($items);
        if ($str) {
          if ($pre = implode(', ', $items)) {
            $pre .= ', ';
          }
          $resource = db_query("SELECT TRIM(c.first_name) first_name, TRIM(c.last_name) last_name, TRIM(e.email) email
          FROM {civicrm_contact} c, {civicrm_email} e
          WHERE c.id = e.contact_id AND e.email <> '' AND c.is_deleted = 0 AND c.contact_type = 'Individual'
          AND c.first_name IS NOT NULL AND c.last_name IS NOT NULL AND e.email IS NOT NULL
          AND (c.sort_name LIKE '%%%s%%' OR e.email LIKE '%%%s%%')
          ORDER BY c.sort_name
          LIMIT 30", $str, $str, $str);
          while ($c = db_fetch_array($resource)) {
            $email = '"' . $c['first_name'] . ' ' . $c['last_name'] . '" <' . $c['email'] . '>, ';
            $matches[$pre . $email] = $c['last_name'] . ', ' . $c['first_name'] . ' - ' . $c['email'];
          }
        }
        break;

      case 'trip':
        module_load_include('inc', 'woolman_semester', 'ws_prospectives');
        $resource = db_query("SELECT id, subject, activity_date_time FROM {civicrm_activity}
                              WHERE (subject LIKE '%%%s%%' OR location LIKE '%%%s%%') AND activity_type_id = 33 AND is_current_revision = 1 AND is_deleted <> 1
                              LIMIT 30", $str, $str);
        while ($row = db_fetch_array($resource)) {
          $name = ltrim($row['subject'], 'Outreach Trip to ');
          $matches[$name.' (ID#' . $row['id'].')'] = $name.' on '  ._ws_admissions_date_format($row);
        }
        break;
    }
  }
  exit(drupal_json($matches));
}

/**
 * Outputs CiviCRM custom field group to Drupal FAPI
 * @param $group: id of field group
 * @param $entity_id: to find previous submission for setting default values
 * @param $submission_id: id of previous submission for setting default values: use this
 *        instead of $entity_id if the custom fieldgroup is multi-valued and you want to specify one of the submissions
 * @return Array ready to be used by FAPI as a form, or as part of a larger form
 * NOTE: Not all civicrm field types and attributes are supported
 */
function woolman_custom_field_group_fapi($group, $entity_id=NULL, $submission_id=NULL) {
  $fields = $option_group = $options = $form = $vals = array();
  $field_types = array(
    'Select' => 'select',
    'Multi-Select' => 'checkboxes',
    'Radio' => 'radios',
    'CheckBox' => 'checkboxes',
    'Text' => 'textfield',
    'TextArea' => 'textarea',
    'RichTextEditor' => 'textarea',
    'Select Date' => 'date',
    'Link' => 'textfield',
  );
  // Get form info
  $info = db_query("SELECT * FROM {civicrm_custom_group}
                    WHERE id = %d", $group);
  $info = db_fetch_array($info);

  // Get fields
  $query = db_query(" SELECT * FROM {civicrm_custom_field}
                      WHERE custom_group_id = %d AND is_active <> 0 AND is_view <> 1
                      ORDER BY weight", $group);
  while ($row = db_fetch_array($query)) {
    // Ignore unknown field types
    if (!$field_types[$row['html_type']]) {
      continue;
    }
    $fields[$row['column_name']] = $row;
    if ($row['option_group_id']) {
      $option_group[] = $row['option_group_id'];
    }
  }

  // Get options
  if ($option_group) {
    $query = db_query(" SELECT option_group_id as id, label, value, is_default FROM {civicrm_option_value}
                        WHERE option_group_id IN (".implode(',',$option_group).") AND is_active = 1
                        ORDER BY weight");
    while ($row = db_fetch_array($query)) {
      $options[$row['id']][$row['value']] = $row['label'];
      if ($row['is_default']) {
        $defaults[$row['id']][] = $row['value'];
      }
    }
  }

  // Get existing submission
  if ($entity_id || $submission_id) {
    $q = 'SELECT * FROM {' . $info['table_name'].'} ';
    if ($submission_id) {
      $query = db_query($q.'WHERE id = %d', $submission_id);
    }
    else {
      $query = db_query($q.'WHERE entity_id = %d', $entity_id);
    }
    $vals = db_fetch_array($query);
  }

  foreach ($fields as $id => $f) {
    $form[$id] = array(
      '#type' => $field_types[$f['html_type']],
      '#title' => $f['label'],
      '#required' => $f['is_required'],
      '#description' => $f['help_pre'],
      '#options' => $options[$f['option_group_id']],
      '#rows' => $f['note_rows'],
      '#cols' => $f['note_columns'],
      '#default_value' => NULL,
    );
    if ($form[$id]['#type'] == 'textfield') {
      $form[$id]['#maxlength'] = $f['text_length'];
      $form[$id]['#size'] = (($f['text_length'] > 60 || $f['text_length'] < 1) ? 60 : $f['text_length']);
    }
    if ($f['data_type'] == 'Boolean') {
      $form[$id]['#options'] = array(1 => 'Yes', 0 => 'No');
    }
    // Three ways to get a default value -- previous submission has top priority
    if ($vals) {
      $form[$id]['#default_value'] = trim($vals[$id], '');
    }
    elseif (!empty($f['default_value'])) {
      $form[$id]['#default_value'] = trim($f['default_value'], '');
    }
    elseif (!empty($f['option_group_id'])) {
      $form[$id]['#default_value'] = $defaults[$f['option_group_id']];
    }
    // Turn multi-valued default into an array
    if (!is_array($form[$id]['#default_value']) && strpos($form[$id]['#default_value'], '')) {
      $form[$id]['#default_value'] = explode('', $form[$id]['#default_value']);
    }
    // Format dates properly
    if ($form[$id]['#default_value'] && $form[$id]['#type'] == 'date') {
      $form[$id]['#default_value'] = woolman_date_string_to_array($form[$id]['#default_value']);
    }
    // Make sure checkbox default value is an array
    if ($form[$id]['#type'] == 'checkboxes' && !is_array($form[$id]['#default_value'])) {
      $form[$id]['#default_value'] = array($form[$id]['#default_value']);
    }
  }
  return $form;
}

/**
 * Helper function for submit handlers to put custom fieldgroup submission from FAPI into the CiviCRM DB
 */
function woolman_custom_field_group_fapi_submit($vals, $group, $entity_id) {
  // Get form info
  $query = db_query("SELECT * FROM {civicrm_custom_group}
                      WHERE id = %d", $group);
  $info = db_fetch_array($query);

  // These field types requre a value separator
  $multi_types = array('Multi-Select', 'CheckBox');
  $multi = array();
  $query = db_query("SELECT * FROM {civicrm_custom_field}
                      WHERE custom_group_id = %d", $group);
  while ($row = db_fetch_array($query)) {
    $fields[$row['column_name']] = $row;
    // Note which fields are multi-valued
    if ($multi_types[$row['html_type']]) {
      $multi[] = $row['column_name'];
    }
  }

  $update = '';
  $into = 'entity_id';
  $set = $entity_id;
  $values = array();

  foreach ($fields as $id => $f) {
    $placeholder = "'%s'";
    if ($val = woolman_aval($vals, $id)) {
      if ($f['html_type'] == 'Select Date' && is_array($val)) {
        $val = woolman_date_array_to_string($val, '-');
      }
      if (is_array($val) || in_array($id, $multi)) {
        $val = woolman_multivalue_str($val);
      }
      elseif (is_numeric($val)) {
        $placeholder = '%d';
      }
    }
    elseif (!array_key_exists($id, $vals)) {
      continue;
    }
    if ($update) {
      $update .= ', ';
    }
    $update .= $id.' = ' . $placeholder;
    $into .= ', ' . $id;
    $set .= ', ' . $placeholder;
    $values[] = $val;
  }

  // Search for existing submission if this fieldgroup is single-valued
  if (!$info['is_multiple']) {
    $query = db_query('SELECT id FROM {' . $info['table_name'].'}
                          WHERE entity_id = %d', $entity_id);

    if ($existing = db_fetch_array($query)) {
      if (db_query("UPDATE {".$info['table_name']."}
                    SET $update
                    WHERE id = ".$existing['id'], $values)) {
        return 'updated existing record';
      }
      return;
    }
  }
  // Insert new record
  if (db_query("INSERT INTO {".$info['table_name']."} ($into) VALUES ($set)", $values)) {
    return 'created new record';
  }
}

/**
 * Drupal doesn't have a theme function for a single table row. Here it is:
 * @see theme.inc
 */
function woolman_table_row($row) {
  $attributes = array();

  // Check if we're dealing with a simple or complex row
  if (isset($row['data'])) {
    foreach ($row as $key => $value) {
      if ($key == 'data') {
        $cells = $value;
      }
      else {
        $attributes[$key] = $value;
      }
    }
  }
  else {
    $cells = $row;
  }
  if (count($cells)) {
    // Build row
    $output .= ' <tr'. drupal_attributes($attributes) .'>';
    $i = 0;
    foreach ($cells as $cell) {
      $output .= _theme_table_cell($cell);
    }
    $output .= " </tr>\n";
  }
  return $output;
}

/**
 * Processor for woolman_num_to_words()
 * Recursive fn, converts three digits per pass
 */
function woolman_convert_triples($num, $tri = 0) {
  $ones = array('', ' one', ' two', ' three', ' four', ' five', ' six', ' seven', ' eight', ' nine', ' ten',
    ' eleven', ' twelve', ' thirteen', ' fourteen', ' fifteen', ' sixteen', ' seventeen', ' eighteen', ' nineteen');

  $tens = array('', '', ' twenty', ' thirty', ' forty', ' fifty', ' sixty', ' seventy', ' eighty', ' ninety');

  $triplets = array('', ' thousand', ' million', ' billion', ' trillion', ' quadrillion', ' quintillion',
    ' sextillion', ' septillion', ' octillion', ' nonillion');

  // chunk the number, ...rxyy
  $r = (int) ($num / 1000);
  $x = ($num / 100) % 10;
  $y = $num % 100;

  // init the output string
  $str = '';

  // do hundreds
  if ($x > 0) {
    $str = $ones[$x] . ' hundred';
  }
  // do ones and tens
  if ($y < 20) {
    $str .= $ones[$y];
  }
  else {
    $str .= $tens[(int) ($y / 10)] . $ones[$y % 10];
  }
  // add triplet modifier only if there
  // is some output to be modified...
  if ($str != '') {
    $str .= $triplets[$tri];
  }
  // continue recursing?
  if ($r > 0) {
    return woolman_convert_triples($r, $tri + 1) . $str;
  }
  else {
    return $str;
  }
}

/**
 * Convert fraction to english string
 */
function woolman_num_to_words_remainder($rem) {
  if ($rem == .25) {
    return 'a quarter';
  }
  if ($rem == .5) {
    return 'a half';
  }
  if ($rem == .75) {
    return 'three quarters';
  }
}

/**
 * Wrapper for adding/finding/updating contacts in CiviCRM
 * @param $params: array of arrays: keys should be:
                   'contact', 'address', 'phone', 'email'
 * @param $write_mode: 'search', 'do_not_update', 'update_all', 'fill_empty_fields'
 * @param $match_mode: 'none', 'strict' or 'stricter'
 * @param $return: 'return_cid' or 'return_contact'
 * @param $exclude: string: comma separated list of contact ids that this person is not a match for (or subquery which returns ids) {
 * @return array('op' => action taken, 'contact_id', 'contact' => full contact record) {
 */
function _woolman_contact_match($params, $write_mode='update_all', $return='return_cid', $match_mode='strict', $exclude='') {
  $params['contact'] += array('contact_type' => 'Individual');
  $type = $params['contact']['contact_type'];

  if ($type != 'Individual') {
    $match_mode = 'strict';
  }

  if (!empty($params['address']['share_with_contact'])) {
    $result = woolman_civicrm_api('address', 'get', array('contact_id' => $params['address']['share_with_contact'], 'is_primary' => 1));
    if (!empty($result['values'])) {
      $master_address = array_pop($result['values']);
      if (!$master_address['master_id']) {
        $master_address['master_id'] = $master_address['id'];
      }
      unset($master_address['id'], $master_address['contact_id'], $master_address['geo_code_1'], $master_address['geo_code_2']);
      $params['address'] = array($master_address);
    }
    else {
      unset($params['address']);
    }
  }

  if (!empty($params['contact']['contact_id'])) {
    $cid = $params['contact']['contact_id'];
  }
  if (empty($cid) && $match_mode != 'none') {
    if (!empty($params['contact']['user_id'])) {
      $cid = woolman_user_cid($params['contact']['user_id']);
      unset($params['contact']['user_id']);
    }
    elseif (!empty($params['contact']['external_identifier']) || !empty($params['contact']['legal_identifier'])) {
      $type = !empty($params['contact']['external_identifier']) ? 'external' : 'legal';
      $db = db_query("SELECT id FROM {civicrm_contact} WHERE {$type}_identifier = '%s'", $params['contact'][$type . '_identifier']);
      if ($row = db_fetch_array($db)) {
        $cid = $row['id'];
      }
    }
    if (empty($cid)) {
      $join = '';
      $where = " WHERE co.is_deleted <> 1 AND co.contact_type = '%s'";
      $args = array($type);
      $criteria_met = 0;
      if ($exclude) {
        $where .= ' AND co.id NOT IN ('. trim($exclude, ' ,' ) .')';
      }
      if ($type == 'Individual') {
        if (($first_name = $params['contact']['first_name']) || ($nick_name = $params['contact']['nick_name'])) {
          ++$criteria_met;
          $where .= ' AND (';
          if ($first_name) {
            $where .= "co.first_name = '%s' OR co.nick_name = '%s'";
            $args[] = $first_name; $args[] = $first_name;
          }
          if ($nick_name) {
            if ($first_name) {
              $where .= ' OR ';
            }
            $where .= "co.first_name = '%s' OR co.nick_name = '%s'";
            $args[] = $nick_name; $args[] = $nick_name;
          }
          $where .= ')';
        }
        if ($last_name = $params['contact']['last_name']) {
          ++$criteria_met;
          $where .= " AND co.last_name = '%s'";
          $args[] = $last_name;
        }
      }
      elseif ($type == 'Organization' || $type == 'Household') {
        $ent = strtolower($type);
        if ($params['contact'][$ent.'_name']) {
          $criteria_met = 2;
        }
        $where .= " AND co.{$ent}_name = '%s'";
        $args[] = $params['contact'][$ent.'_name'];
      }
      $location = array('email' => array(), 'address' => array(), 'phone' => array());
      foreach ($location as $loc => $x) {
        $field_name = str_replace('address', 'street_address', $loc);
        if (is_array($params[$loc])) {
          foreach ($params[$loc] as $param_data) {
            if (!empty($param_data[$field_name])) {
              $location[$loc][] = $param_data[$field_name];
              $location_data_exists = TRUE;
            }
          }
        }
      }
      if (!empty($location_data_exists)) {
        ++$criteria_met;
        $loc_query = array();
        if ($match_mode == 'stricter' && !empty($params['contact']['birth_date'])) {
          $dob = date('Y-m-d', strtotime($params['contact']['birth_date']));
        }
        foreach ($location as $loc=> $fields) {
          if ($fields) {
            if ($match_mode == 'stricter' && $loc != 'email' && !$params['contact']['birth_date']) {
              continue;
            }
            $field_name = str_replace('address', 'street_address', $loc);
            $join .= " LEFT JOIN {civicrm_$loc} $loc ON $loc.contact_id = co.id";
            foreach ($fields as $field) {
              if ($loc == 'phone') {
                $query = "LEFT(REPLACE(REPLACE(REPLACE(REPLACE(REPLACE(phone.phone, '.', ''), ' ', ''), '-', ''), '(', ''), ')', ''), 10) = '%s'";
                $field = substr(str_replace(array(' ', '-', ')', '(', '.'), '', $field), 0, 10);
              }
              else {
                $query = "$loc.$field_name = '%s'";
              }
              if ($match_mode == 'stricter' && $loc != 'email') {
                $query = "(co.birth_date = '%s' AND $query)";
                $args[] = $dob;
              }
              $loc_query[] = $query;
              $args[] = $field;
            }
          }
        }
        $where .= ' AND ('. implode(' OR ', $loc_query) .')';
      }
      // Did we meet enough criteria for search results to be valid?
      if ($criteria_met >= 3) {
        // Check stricter criteria if in strictist matching mode
        if (!($match_mode == 'stricter'&&!$location['email']&&!$dob)) {
          $db = db_query('SELECT co.id FROM {civicrm_contact} co' . $join . $where . ' LIMIT 0, 1', $args);
          if ($result = db_fetch_array($db)) {
            $cid = $result['id'];
          }
        }
      }
    }
  }

  // Update or create new contact.
  if (!$cid) {
    if ($write_mode == 'search') {
      $op = 'no_create';
    }
    else {
      $result = woolman_civicrm_api('contact', 'create', $params['contact']);
      if (!($cid = $result['id'])) {
        return array('is_error' => TRUE, 'contact_id' => 0, 'op' => 'error');
      }
      $op = 'create';
    }
  }
  elseif ($write_mode == 'do_not_update' || $write_mode == 'search') {
    $op = 'no_update';
  }
  else {
    $op = 'update';
    $params['contact']['contact_id'] = $cid;
    unset($params['contact']['source']);
    if ($write_mode == 'fill_empty_fields') {
      $contact = woolman_civicrm_api('contact', 'get', array('contact_id' => $cid));
      $contact = $contact['values'][$cid];
      foreach ($params['contact'] as $pid => $pvalue) {
        if ($pid != 'contact_id' && $pid != 'contact_type' && !empty($contact[$pid])) {
          unset($params['contact'][$pid]);
        }
      }
      if (!empty($params['address'])) {
        foreach ($params['address'][0] as $pid => $pvalue) {
          if (!empty($contact[$pid])) {
            unset($params['address'][0][$pid]);
          }
        }
      }
      if (!empty($contact['email']) && empty($contact['on_hold'])) {
        unset($params['email'][0]);
      }
      if (!empty($contact['phone'])) {
        unset($params['phone'][0]);
      }
    }
    woolman_civicrm_api('contact', 'create', $params['contact']);
  }
  if ($op != 'no_update' && $write_mode != 'search' && $cid && (!empty($params['email']) || !empty($params['address']) || !empty($params['phone']) || !empty($params['website']))) {
    foreach (array('address', 'email', 'phone', 'website') as $location) {
      if (!empty($params[$location]) && is_array($params[$location])) {
        $existing = woolman_civicrm_api($location, 'get', array('contact_id' => $cid), 'values');
        foreach ($params[$location] as $address_params) {
          $address_params['contact_id'] = $cid;
          if (empty($address_params['location_type_id']) && empty($address_params['location_type']) && $location != 'website') {
            $address_params['location_type_id'] = 1;
          }
          if ($location == 'phone' && empty($address_params['phone_type_id'])) {
            $address_params['phone_type_id'] = 1;
          }
          elseif ($location == 'website' && empty($address_params['website_type_id'])) {
            $address_params['website_type_id'] = 1;
          }
          $same = FALSE;
          foreach ($existing as $loc) {
            if ($loc['location_type_id'] == $address_params['location_type_id'] && ($location != 'phone' || $address_params['phone_type_id'] == $loc['phone_type_id'])) {
              $address_params['id'] = $loc['id'];
              // If all params match existing, no need to update
              $same = TRUE;
              foreach ($address_params as $k => $v) {
                if (woolman_aval($loc, $k) != $v) {
                  $same = FALSE;
                  break;
                }
              }
              // Prevent recursively-shared addresses
              if (woolman_aval($address_params, 'master_id') == $loc['id']) {
                $address_params['master_id'] = 'null';
              }
              break;
            }
          }
          if (!$same) {
            woolman_civicrm_api($location, 'create', $address_params);
          }
        }
      }
    }
  }
  if ($return == 'return_cid') {
    return array('op' => $op, 'contact_id' => $cid, 'is_error' => FALSE);
  }
  $contact = woolman_civicrm_api('contact', 'get', array('contact_id' => $cid));
  return array('op' => $op, 'contact_id' => $cid, 'contact' => $contact['values'][$cid], 'is_error' => FALSE);
}

/**
 * This is legacy code used in "Student Project" nodes
 * Generate list of semesters from 2005 to *now*
 * Please resist the temptation to limit this list to more recent semesters here,
 * as it will break the display of older entries. Try using hook_form_alter on the node form instead.
 */
function woolman_semesters_list_as_integers() {
  $month = date('m');
  $semester = ($month < 9 ? 1 : 2);
  $year = date('Y');

  $y = 2005;
  $s = 1;
  $value = array();

  while ($y <= $year){

    $sem = ($s == 1 ? 'Spring ' : 'Fall ');

    $value[10*$y+$s] = $sem.$y;

    $s++;
    if ($s >= 3){
      $s = 1;
      $y++;
    }
    if ($y >= $year && $s > $semester) $y++;
  }

  return $value;
}
