<?php

function woolman_dupe_query($obj, $type, &$query) {
  $rules = array(
    'IndividualStrict' => array(
      'internal' => 'individual_strict_internal',
      'record' => 'individual_strict_record'
    ),
    'IndividualFuzzy' => array(
      'internal' => 'individual_fuzzy_internal',
      'record' => ''
    ),
  );
  $rule_type = empty($obj->params) ? 'internal' : 'record';
  $rule_name = str_replace(' ', '', $obj->name);

  // If its a rule that we've overridden
  if (array_key_exists($rule_name, $rules) && $rule = $rules[$rule_name][$rule_type]) {

    $obj->threshold = 5;
    if (!($q = call_user_func('woolman_dedupe_' . $rule, $obj))) {
      $query = array();
      return;
    }

    // For internal queries: verify t1 and t2 are different and prevent mirrored results
    if ($rule_type == 'internal') {
      $q .= " AND t1.id < t2.id AND t1.is_deleted = 0 AND t2.is_deleted = 0";
      // Make sure to respect the contactIds constraints
      if (!empty($obj->contactIds)) {
        $cids = implode(',', $obj->contactIds);
        $q .= " AND (t1.id IN($cids) OR t2.id IN($cids))";
      }
    }
    $query = array('civicrm_contact.display_name.5' => $q);
  }
}


function woolman_dedupe_individual_strict_internal($obj) {
  return "
    SELECT t1.id id1, t2.id id2, 5 weight
    FROM civicrm_contact t1
    JOIN civicrm_contact t2 ON (t1.first_name = t2.first_name OR t1.nick_name = t2.first_name OR t1.first_name = t2.nick_name)
    LEFT JOIN civicrm_email e1 ON e1.contact_id = t1.id
    LEFT JOIN civicrm_email e2 ON e2.contact_id = t2.id
    WHERE t1.contact_type = 'Individual'
      AND t2.contact_type = 'Individual'
      AND (t1.last_name = t2.last_name OR e1.email = e2.email)
  ";
}


function woolman_dedupe_individual_fuzzy_internal($obj) {
  return "
    SELECT t1.id id1, t2.id id2, 5 weight
    FROM civicrm_contact t1
    JOIN civicrm_contact t2 ON t2.contact_type = 'Individual'
      AND (SUBSTRING_INDEX(t1.first_name, ' ', 1) = SUBSTRING_INDEX(t2.first_name, ' ', 1)
        OR SUBSTRING_INDEX(t1.nick_name, ' ', 1) = SUBSTRING_INDEX(t2.first_name, ' ', 1)
        OR SUBSTRING_INDEX(t1.first_name, ' ', 1) = SUBSTRING_INDEX(t2.nick_name, ' ', 1)
        OR SUBSTRING_INDEX(t1.first_name, ' ', -1) = SUBSTRING_INDEX(t2.first_name, ' ', -1)
        OR SUBSTRING_INDEX(t1.nick_name, ' ', -1) = SUBSTRING_INDEX(t2.first_name, ' ', -1)
        OR SUBSTRING_INDEX(t1.first_name, ' ', -1) = SUBSTRING_INDEX(t2.nick_name, ' ', -1)
        OR t1.nick_name = t2.middle_name
        OR t1.middle_name = t2.nick_name
      )
    LEFT JOIN civicrm_email e1 ON e1.contact_id = t1.id
    LEFT JOIN civicrm_email e2 ON e2.contact_id = t2.id
    WHERE t1.contact_type = 'Individual'
      AND (
        (SUBSTRING_INDEX(t1.last_name, ' ', 1) = SUBSTRING_INDEX(t2.last_name, ' ', 1)
          OR SUBSTRING_INDEX(t1.last_name, ' ', -1) = SUBSTRING_INDEX(t2.last_name, ' ', -1)
          OR SUBSTRING_INDEX(t1.last_name, '-', 1) = SUBSTRING_INDEX(t2.last_name, '-', 1)
          OR SUBSTRING_INDEX(t1.last_name, '-', -1) = SUBSTRING_INDEX(t2.last_name, '-', -1))
        OR e1.email = e2.email
        OR (SUBSTRING_INDEX(e1.email, '@', 1) = SUBSTRING_INDEX(e2.email, '@', 1) AND t1.last_name = t2.last_name))
  ";
}


function woolman_dedupe_individual_strict_record($obj) {
  $addr_abbr = array(
    ',' => ' ',
    '.' => ' ',
    '-' => ' ',
    ':' => ' ',
    ';' => ' ',
    '#' => ' ',
    ' street ' => ' st ',
    ' road ' => ' rd ',
    ' boulevard ' => ' blvd ',
    ' avenue ' => ' ave ',
    ' terrace ' => ' ter ',
    ' parkway ' => ' pkwy ',
    ' west ' => ' w ',
    ' east ' => ' e ',
    ' north ' => ' n ',
    ' south ' => ' s ',
    ' apartment ' => ' ',
    ' apt ' => ' ',
    ' place ' => ' pl ',
    ' penthouse ' => ' ph ',
    ' ' => '',
  );
  $join = '';
  $where = "WHERE t1.is_deleted = 0 AND t1.contact_type = 'Individual'";
  $loc_query = array();
  $criteria_met = 0;
  $p = $obj->params;
  foreach (array('first', 'last', 'nick') as $n) {
    ${$n . '_name'} = CRM_Core_DAO::escapeString(trim(CRM_Utils_Array::value($n . '_name', $p['civicrm_contact'], '')));
  }
  if ($first_name || $nick_name) {
    ++$criteria_met;
    $where .= "\nAND (";
    if ($first_name) {
      $where .= "t1.first_name = '$first_name' OR t1.nick_name = '$first_name'";
    }
    if ($nick_name && $nick_name != $first_name) {
      $where .= ($first_name ? ' OR ' : '') ."t1.first_name = '$nick_name' OR t1.nick_name = '$nick_name'";
    }
    $where .= ')';
  }
  if ($last_name) {
    ++$criteria_met;
    $where .= "\nAND t1.last_name = '$last_name'";
  }
  if ($birth_date = CRM_Utils_Array::value('birth_date', $p['civicrm_contact'], '')) {
    $loc_query[] = "t1.birth_date = '" . date('Y-m-d', strtotime($p['civicrm_contact']['birth_date'])) . "'";
  }

  foreach (array('email', 'address', 'phone') as $loc) {
    $field_name = str_replace('address', 'street_address', $loc);
    if ($field = CRM_Core_DAO::escapeString(trim(CRM_Utils_Array::value($field_name, $p['civicrm_' . $loc], '')))) {
      $join .= "\nLEFT JOIN civicrm_$loc $loc ON $loc.contact_id = t1.id";
      if ($loc == 'phone') {
        $unwanted_phone_chars = array(' ', '-', ')', '(', '.', '/');
        $field = substr(str_replace($unwanted_phone_chars, '', $field), 0, 10);
        $query = 'phone.phone';
        foreach ($unwanted_phone_chars as $char) {
          $query = "REPLACE($query, '$char', '')";
        }
        $query = "LEFT($query, 10) = '$field'";
      }
      else {
        $query = "$loc . $field_name = '$field'";
      }
      $loc_query[] = $query;
    }
  }
  if (!empty($loc_query)) {
    ++$criteria_met;
    $where .= "\nAND (". implode(' OR ', $loc_query) .')';
  }
  // Did we meet enough criteria for search results to be valid?
  if ($criteria_met >= 3) {
    return "SELECT t1.id id1, 5 weight\nFROM civicrm_contact t1 $join \n$where";
  }
}
