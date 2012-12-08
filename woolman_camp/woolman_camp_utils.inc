<?php

civicrm_initialize();

/**
 * Fetch camp programs (civi event types) managed by this module.
 * TODO: Eventually we will store this in the db and provide a UI
 */
function woolman_camp_event_types() {
  return array(
    8 => array(
      'event_type' => 'Camp Woolman',
      'abbr' => 'sfc',
      'age_min' => 9,
      'age_max' => 14,
    ),
    9 => array(
      'event_type' => 'Teen Leadership Camp',
      'abbr' => 'tlc',
      'age_min' => 15,
      'age_max' => 16,
    ),
  );
}

/**
 * Find user's family members
 * Loads most recent camp attendance for each child
 */
function woolman_camp_fetch_relations() {
  static $found = array();
  if ($found) {
    return $found;
  }

  $cid = woolman_user_cid();
  $children = $spouse = $e_contact = $kids = array();
  $past_reg = FALSE;
  if ($relations = woolman_civicrm_api('relationship', 'get', array('contact_id' => $cid), 'values')) {
    //Load contacts and eliminate duplicates
    foreach ($relations as $rel) {
      if ($rel['rtype'] == 'b_a' && $rel['relationship_type_id'] == 1) {
        $children[$rel['cid']] = woolman_contact_get($rel['cid']);
        foreach ($rel as $key => $value) {
          if (substr($key, 0, 8) == 'custom_8')
            $children[$rel['cid']]['relationship_custom'] = $value;
        }
        if ($events = woolman_civicrm_api('participant', 'get', array('contact_id' => $rel['cid']), 'values')) {
          $prior = array();
          foreach ($events as $event) {
            if ($event['event_type'] == 'Sierra Friends Camp' || $event['event_type'] == 'Teen Leadership Camp') {
              $prior[$event['event_start_date'] . $event['event_id']] = $event;
            }
          }
          if (!empty($prior)) {
            krsort($prior);
            $children[$rel['cid']]['prior_camp'] = $prior;
            $past_reg = TRUE;
          }
        }
      }
      elseif ($rel['relationship_type_id'] == 2) {
        //If multiple spouses, pick most active relationship
        if ($spouse) {
          if (!$rel['is_active'] || ($spouse['is_active'] && ($spouse['relationship_custom']=='Married' || $spouse['relationship_custom']=='Partners')) || ($rel['start_date'] && $spouse['start_date'] && ($rel['start_date'] < $spouse['start_date'])) || $rel['end_date'])
            continue;
        }
        $spouse = woolman_camp_contact_get($rel['cid']);
        foreach ($rel as $key => $value) {
          if (substr($key, 0, 8) == 'custom_9')
            $spouse['relationship_custom'] = $value;
        }
        if (!$rel['is_active'])
          $spouse['relationship_custom'] = 0;
      }
    }
    if ($children) {
      //get emergency contact info from kids
      $now = date('Y-m-d');
      $types = array('Emergency Contact is');
      foreach ($children as $child) {
        foreach (woolman_civicrm_api('relationship', 'get', array('contact_id' => $child['contact_id']), 'values') as $contact) {
          if ($contact['is_active'] && $contact['relation'] == 'Emergency Contact is' && (!$contact['end_date'] || $contact['end_date']>$now) && $contact['cid'] != $cid && $contact['cid'] != woolman_aval($spoues, 'contact_id')) {
            $e_contact = woolman_camp_contact_get($contact['cid']);
            break 2;
          }
        }
      }
    }
    if ($spouse) {
      //load spouse's relation to kids
      foreach (woolman_civicrm_api('relationship', 'get', array('contact_id' => $spouse['contact_id']), 'values') as $rel) {
        if ($rel['rtype'] == 'b_a' && $rel['relationship_type_id'] == 1 && !empty($children[$rel['cid']])) {
          foreach ($rel as $key => $value) {
            if (substr($key, 0, 8) == 'custom_8')
              $children[$rel['cid']]['relationship_to_spouse'] = $value;
          }
        }
      }
    }
    //sort kids by age
    foreach ($children as $id => $child) {
      $kids[woolman_aval($child, 'birth_date', 0) . $id] = $child;
    }
    krsort($kids);
    $found = array(array_values($kids), $spouse, $e_contact, $past_reg);
  }
  return $found;
}

/**
 * Calculates the age of a camper as of July 1 of a given year
 * If a cid is provided, adjust for any age exception
 */
function woolman_camp_age($dob, $year, $cid = NULL) {
  if ($cid && is_numeric($cid)) {
    $query = db_query("SELECT delta FROM {woolman_camp_age_exception} WHERE year = %d AND cid = %d", $year, $cid);
    if ($exception = db_fetch_object($query)) {
      $year += $exception->delta;
    }
  }
  $camp_date = strtotime($year.'-07-01');
  $birth_date = strtotime($dob);
  return (int)(($camp_date-$birth_date)/60/60/24/365.25);
}

/**
 * Check if a given age is within the limits of a particluar program
 */
function woolman_camp_age_range($age, $program_id, $padding = 0) {
  $programs = woolman_camp_event_types();
  $min = $programs[$program_id]['age_min'] + $padding;
  $max = $programs[$program_id]['age_max'] - $padding;
  return $age >= $min && $age <= $max;
}

/**
 * Gets session dates and attendance numbers
 * Gets campers: by default a simple count, or optionally get details of each camper
 */
function woolman_camp_get_sessions($year, $get_campers = FALSE) {
  $programs = woolman_camp_event_types();
  $sessions = array();
  $query = db_query(
    "SELECT ev.id, ev.title, ev.start_date, ev.end_date, ev.event_type_id, ev.max_participants, ps.price_set_id, COUNT(pa.id) AS campers, COUNT(boys.id) AS boys, COUNT(girls.id) AS girls, DATEDIFF(ev.end_date, ev.start_date) as days
    FROM {civicrm_event} ev
    LEFT JOIN {civicrm_price_set_entity} ps ON ps.entity_table = 'civicrm_event' AND ps.entity_id = ev.id
    LEFT JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE is_counted = 1)
    LEFT JOIN {civicrm_contact} girls ON girls.id = pa.contact_id AND girls.gender_id = 1
    LEFT JOIN {civicrm_contact} boys ON boys.id = pa.contact_id AND boys.gender_id = 2
    WHERE ev.event_type_id IN(" . implode(',', array_keys($programs)) . ") AND YEAR(ev.start_date) = %d AND ev.is_active = 1 AND ev.is_template <> 1
    GROUP BY ev.id
    ORDER BY ev.event_type_id, ev.start_date
    ", $year);
  while($row = db_fetch_array($query)) {
    if (!$row['max_participants']) {
      $row['max_participants'] = 30;
      $row['max_participants_not_set'] = TRUE;
    }
    $row['full'] = ($row['campers'] >= $row['max_participants']);
    $row['weeks'] = (int)round($row['days']/7);
    if ($get_campers) {
      $row['campers'] = array();
    }
    $sessions[$row['id']] = $row;
  }
  if ($get_campers && $sessions) {
    $query = db_query(
      "SELECT st.name AS status, par.status_id, con.first_name, con.nick_name, con.last_name, con.middle_name, con.gender_id, con.birth_date, par.fee_amount, par.contact_id, ev.id AS event_id, par.id AS participant_id, st.class, addr.city, YEAR(latest.start_date) AS camper_since, med.date_form_submitted_38 AS medform_date
      FROM {civicrm_participant} par
      INNER JOIN {civicrm_contact} con ON con.id = par.contact_id
      INNER JOIN {civicrm_participant_status_type} st ON st.id = par.status_id
      INNER JOIN {civicrm_event} ev ON par.event_id = ev.id
      LEFT JOIN {civicrm_value_medical_info_9} med ON med.entity_id = par.contact_id
      LEFT JOIN {civicrm_event} latest ON latest.start_date =
        (SELECT MIN(old.start_date) FROM {civicrm_event} old WHERE old.id IN
          (SELECT op.event_id FROM {civicrm_participant} op WHERE op.contact_id = par.contact_id))
      LEFT JOIN {civicrm_address} addr ON addr.contact_id = par.contact_id AND addr.is_primary = 1
      WHERE par.is_test <> 1 AND con.is_deleted <> 1 AND ev.id IN (".implode(',', array_keys($sessions)).")");
    while($row = db_fetch_array($query)) {
      $id = $row['event_id'];
      unset($row['event_id']);
      $row['age'] = woolman_camp_age($row['birth_date'], $year);
      $sessions[$id]['campers'][$row['participant_id'] . '-' . $row['status_id']] = $row;
    }
  }
  return $sessions;
}

/**
 * Gets registration activity from current user
 */
function woolman_camp_get_reg_act() {
  static $act = array();
  if ($act) {
    return $act;
  }
  $query = db_query('
    SELECT id, status_id
    FROM {civicrm_activity}
    WHERE source_contact_id = %d AND activity_type_id = 56 AND is_deleted <> 1 AND is_test <> 1
    AND is_current_revision = 1 AND YEAR(activity_date_time + INTERVAL 4 month) = ' . woolman_camp_next_year(),
    woolman_user_cid());
  if ($act = db_fetch_array($query)) {
    return $act;
  }
  return array('id' => 0, 'status_id' => 0);
}

/**
 * Updates registration activity status
 * Creates activity if it does not exist
 */
function woolman_camp_update_reg_act($status) {
  $act = woolman_camp_get_reg_act();
  if (!empty($act['id'])) {
    if ($status != $act['status_id']) {
      woolman_civicrm_api('activity', 'create', array(
        'id' => $act['id'],
        'status_id' => $status,
      ));
    }
  }
  else {
    $cid = woolman_user_cid();
    // Create new registration activity
    woolman_civicrm_api('activity', 'create', array(
      'activity_type_id' => 56,
      'source_contact_id' => $cid,
      'target_contact_id' => array($cid),
      'subject' => 'Registration for camp ' . woolman_camp_next_year(),
      'status_id' => $status,
    ));

    // Alert staff of new registration
    $message = array(
      'subject' => 'New camp registration by ' . woolman_name('display'),
      'body' => woolman_name('display') . ' just started the registration process.',
    );
    drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
  }
}

/**
 * Gets camp rates for a given year
 */
function woolman_camp_get_rates($year) {
  static $rates = array();
  if (!isset($rates[$year])) {
    $db = db_query('SELECT * FROM {woolman_camp_rates} WHERE year = %d', $year);
    $rates[$year] = db_fetch_array($db);
    unset($rates[$year]['year']);
  }
  return $rates[$year];
}

/**
 * Retrieve payments for a family
 */
function woolman_camp_get_payments($cid, $year) {
  $payments = array();
  $query = db_query("
    SELECT con.display_name, pay.receive_date, pay.total_amount, pay.check_number, ov.label AS type
    FROM {civicrm_contact} con, {civicrm_contribution} pay
    LEFT JOIN {civicrm_option_value} ov ON ov.option_group_id = 10 AND ov.value = pay.payment_instrument_id
    WHERE con.id = pay.contact_id AND pay.source = %d AND pay.contribution_type_id = 5 AND pay.is_test = 0 AND (con.id = %d OR pay.id IN (SELECT sof.contribution_id FROM {civicrm_contribution_soft} sof WHERE sof.contact_id = %d)) AND pay.contribution_status_id = 1
    ORDER BY pay.receive_date", $year, $cid, $cid);
  while($row = db_fetch_array($query)) {
    if ($row['check_number'] && $row['type'] == 'Check') {
      $row['method'] .= 'Check #' . $row['check_number'];
    }
    else {
      $row['method'] = $row['type'];
    }
    $payments[] = $row;
  }
  return $payments;
}

/**
 * Calculates payments, fees and discounts for a family
 */
function woolman_camp_calc_fees($parent_cid, $year) {
  $programs = woolman_camp_event_types();
  $camp_and_shuttle = array_merge(array(10), array_keys($programs));
  if (!($rates = woolman_camp_get_rates($year))) {
    return;
  }
  $ret = array(
    'total_fees' => 0,
    'total_discount' => 0,
    'total_paid' => 0,
    'rates' => $rates,
    'shuttles' => 0,
    'discounts' => array(),
    'campers' => array(),
    'payments' => woolman_camp_get_payments($parent_cid, $year),
  );
  $discount = 0;
  $counted = array();
  
  // Calculate total paid
  foreach ($ret['payments'] as $p) {
    $ret['total_paid'] += $p['total_amount'];
  }

  // Get campers, sessions & shuttles
  $db = db_query('
    SELECT st.name AS status, st.class, st.is_counted, con.first_name, con.nick_name, con.last_name, con.gender_id, ev.id AS event_id, ev.title, pa.contact_id, ev.start_date, ev.end_date, ev.event_type_id, DATEDIFF(ev.end_date, ev.start_date) AS days, pa.register_date, pa.fee_amount, pa.id AS pid, med.date_form_submitted_38 as medform_date
    FROM {civicrm_event} ev
    INNER JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE class <> \'Negative\')
    INNER JOIN {civicrm_contact} con ON con.id = pa.contact_id
    INNER JOIN {civicrm_participant_status_type} st ON st.id = pa.status_id
    LEFT JOIN {civicrm_value_medical_info_9} med ON med.entity_id = pa.contact_id
    WHERE con.is_deleted <> 1 AND ev.event_type_id IN (' . implode(',', $camp_and_shuttle) . ') AND YEAR(ev.start_date) = %d AND ev.is_active = 1 AND ev.is_template <> 1 AND pa.is_test <> 1 AND (pa.contact_id IN (SELECT contact_id FROM {civicrm_participant} WHERE id IN (SELECT entity_id FROM {civicrm_value_registration_questions_7} WHERE parent_15 = %d)) OR pa.contact_id = %d)
    ORDER BY con.birth_date DESC', $year, $parent_cid, $parent_cid);
  while ($row=db_fetch_array($db)) {
    if (!$row['event_id'] || !$row['contact_id']) {
      continue;
    }
    $cid = $row['contact_id'];
    //Shuttle
    if ($row['event_type_id'] == 10) {
      if ($row['is_counted'] && !isset($ret['campers'][$cid]['shuttle'][$row['event_id']])) {
        ++$ret['shuttles'];
        $ret['total_fees'] += $rates['shuttle_price'];
        if ($cid != $parent_cid) {
          $ret['campers'][$cid]['shuttle'][$row['pid']] = array(
            'title' => $row['title'],
            'start_date' => $row['start_date'],
            'event_id' => $row['event_id'],
            'pid' => $row['pid'],
            'fee_amount' => $row['fee_amount'],
          );
        }
      }
      continue;
    }
    if ($row['is_counted']) {
      $counted[$cid] = $cid;
    }
    if (empty($register_date) || $row['register_date'] < $register_date) {
      $register_date = $row['register_date'];
    }
    if (empty($ret['campers'][$cid]['register_date']) || ($row['register_date'] && $row['register_date'] < $ret['campers'][$cid]['register_date'])) {
      $ret['campers'][$cid]['register_date'] = $row['register_date'];
    }
    $row['weeks'] = $weeks = (int) round($row['days'] / 7);
    $camp = $ret['campers'][$cid]['camp'] = ($row['event_type_id']==8?'sfc':'tlc');
    $row['price'] = $rates[$camp.'_price']*$weeks;
    $ret['total_fees'] += $row['price'];
    $ret[$camp.'_weeks'] += $weeks;
    $ret['campers'][$cid]['weeks'] += $weeks;
    $ret['campers'][$cid]['first_name'] = $row['nick_name'] ? $row['nick_name'] : $row['first_name'];
    $ret['campers'][$cid]['last_name'] = $row['last_name'];
    $ret['campers'][$cid]['gender_id'] = $row['gender_id'];
    $ret['campers'][$cid]['medform_date'] = $row['medform_date'];
    $ret['campers'][$cid]['total_paid'] += $row['fee_amount'];
    if ($ret['campers'][$cid]['session_names'])
      $ret['campers'][$cid]['session_names'] .= ', ';
    $ret['campers'][$cid]['session_names'] .= str_replace($year,'',$row['title']);
    if ($row['status'] == 'Pending from pay later' || $row['status'] == 'Registered') {
      $ret['total_approved'] += $row['weeks'];
      $row['message'] = $row['fee_amount'] >= ($rates['deposit']*$row['weeks']) ? 'Registered.' : 'Registration approved. Please pay deposit to reserve your space in this session.';
    }
    elseif ($row['status'] == 'Awaiting approval') {
      $ret['total_pending'] += $row['weeks'];
      $row['message'] = 'Our staff are reviewing your application and will contact you soon.';
    }
    elseif ($row['status'] == 'On waitlist') {
      $ret['total_waitlist'] += $row['weeks'];
      $row['message'] = 'On waitlist. We will contact you as soon as space becomes available.';
    }
    $row['camp'] = $row['event_type_id'] == 8 ? 'sfc' : 'tlc';
    unset($row['nick_name'],$row['first_name'],$row['contact_id'],$row['gender_id'],$row['status_id'],$row['medform_date']);
    $ret['campers'][$cid]['sessions'][$row['pid']] = $row;
  }
  if (empty($ret['campers'])) {
    return;
  }
  foreach ($ret['campers'] as $camper) {
    // Multi-week discount
    $weeks = $camper['weeks'];
    if (($multi_week_discount = ($rates['sfc_multi_week_discount']*$weeks)) && $camper['camp']=='sfc' && $weeks>1) {
      $ret['total_discount'] += $multi_week_discount;
      if (empty($ret['discounts']['Multi-Week Discount'])) {
        $ret['discounts']['Multi-Week Discount'] = array(
          'date' => $register_date,
          'amount' => 0,
        );
      }
      $ret['discounts']['Multi-Week Discount']['amount'] += $multi_week_discount;
    }
    // Early registration discount
    if ($rates['early_discount'] && (substr($rates['early_date'],0,10) >= substr($camper['register_date'],0,10))) {
      $ret['total_discount'] += $rates['early_discount'];
      if (empty($ret['discounts']['Early Registration'])) {
        $ret['discounts']['Early Registration'] = array(
          'date' => $register_date,
          'amount' => 0,
        );
      }
      $ret['discounts']['Early Registration']['amount'] += $rates['early_discount'];
    }
  }
  // Family discount -- check if there's more than one child registered
  if ($rates['family_discount'] && ($count=count($counted)) > 1) {
    $ret['total_discount'] += $rates['family_discount'] * $count;
    $ret['discounts']['Family Discount'] = array(
      'amount' => $rates['family_discount'] * $count,
      'date' => $register_date
    );
  }
  // Other discounts
  $db = db_query('SELECT * FROM {woolman_camp_discounts} WHERE cid = %d AND year = %d', $parent_cid, $year);
  while ($row=db_fetch_array($db)) {
    unset($row['cid'],$row['year']);
    $ret['total_discount'] += $row['amount'];
    $row['amount'] += $ret['discounts'][$row['type']]['amount'];
    $ret['discounts'][$row['type']] = $row;
  }
  $ret['shuttle_price'] = $ret['shuttles'] * $rates['shuttle_price'];
  $ret['register_date'] = $register_date;
  $ret['deposit'] = $rates['deposit'] * ($ret['sfc_weeks'] + $ret['tlc_weeks']);
  return $ret;
}

/**
 * Format session dates
 * @param str: start date
 * @param str: end date
 * @return Weeks, Date(s)
 */
function woolman_camp_date_format($start, $end=NULL, $return_weeks=TRUE) {
  $start = strtotime($start);
  if ($end) {
    $end = strtotime($end);
    if (date('F',$start) == date('F',$end))
      $date = date('F j', $start).' - '.date('jS',$end);
    else
      $date = date('F j', $start).' - '.date('F j',$end);
    $dates = str_replace('August', 'Aug', $date);
    if (!$return_weeks)
      return $dates;
    $weeks = round(($end-$start)/60/60/24/7);
    return '<strong>' . $weeks.' week'.($weeks==1?'':'s').':</strong> ' . $dates;
  }
  else {
    return date('F jS', $start);
  }
}

/**
 * Loads a contact and parses dob & location info
 */
function woolman_camp_contact_get($cid=NULL) {

  $contact = woolman_contact_get($cid, NULL, TRUE);

  if (!empty($contact['birth_date'])) {
    $contact['dob'] = woolman_date_string_to_array($contact['birth_date']);
  }

  $phones = $emails = array('home' => '', 'work' => '', 'cell' => '', 'primary' => '');
  foreach ($contact['locations']['phone'] as $phone) {
    if ($phone['location_type_id'] == 4 && $phones['primary'] != 'cell') {
      $phones['cell'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'cell';
    }
    if ($phone['location_type_id'] == 1 && $phones['primary'] != 'home') {
      $phones['home'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'home';
    }
    if ($phone['location_type_id'] == 2 && $phones['primary'] != 'work') {
      $phones['work'] = $phone['phone'];
      if ($phone['is_primary']) $phones['primary'] = 'work';
    }
  }
  $contact['phones'] = $phones;

  foreach ($contact['locations']['email'] as $email) {
    if ($email['location_type_id'] == 1 && $emails['primary'] != 'home') {
      $emails['home'] = $email['email'];
      if ($email['is_primary']) {
        $emails['primary'] = 'home';
      }
    }
    if ($email['location_type_id'] == 2 && $emails['primary'] != 'work') {
      $emails['work'] = $email['email'];
      if ($email['is_primary']) {
        $emails['primary'] = 'work';
      }
    }
  }
  $contact['emails'] = $emails;

  return $contact;
}

/**
 * Returns selectable discount types
 */
function woolman_camp_discount_types() {
  return drupal_map_assoc(array(
    'Financial Aid',
    'Discount Voucher',
    'Work-Trade',
    'Tell A Friend Discount',
    'Staff Discount',
    'Family Discount',
    'Account Credit',
  ));
}

/**
 * Fetch the price of an event
 *
 * @param array: $event
 */
function woolman_camp_event_price_get($event) {
  if (empty($event['price_set_id'])) {
    return;
  }
  $price_field = woolman_civicrm_api('price_field', 'get', array('price_set_id' => $event['price_set_id']), 'values');
  foreach ($price_field as $field) {
    $price = woolman_civicrm_api('price_field_value', 'get', array('price_field_id' => $field['id'], 'is_default' => 1), 'values');
    foreach ($price as $value) {
      if (!empty($value['is_default'])) {
        return $value['amount'];
      }
    }
  }
}

/**
 * Fetch the price of an event
 *
 * @param int: $event_id
 * @param int: $price
 */
function woolman_camp_event_price_create($event_id, $price) {
  $price_set = CRM_Core_DAO_singleValueQuery("SELECT price_set_id FROM civicrm_price_set_entity WHERE entity_table = 'civicrm_event' AND entity_id = $event_id LIMIT 1");
  
  $price_field = woolman_civicrm_api('price_field', 'get', array('price_set_id' => $event['price_set_id']), 'values');
  foreach ($price_field as $field) {
    $price = woolman_civicrm_api('price_field_value', 'get', array('price_field_id' => $field['id'], 'is_default' => 1), 'values');
    foreach ($price as $value) {
      if (!empty($value['is_default'])) {
        return $value['amount'];
      }
    }
  }
}

/**
 * Get shuttle dates, info & attendance
 */
function woolman_camp_get_shuttle($year, $get_passengers) {
  $select = 'ev.id AS event_id, ev.title, ev.description, ev.max_participants, ev.start_date, sh.direction_40 AS direction';
  $from = '{civicrm_event} ev LEFT JOIN {civicrm_value_shuttle_info_11} sh ON sh.entity_id = ev.id';
  if ($get_passengers) {
    $select .= ', con.sort_name, con.display_name, con.id AS contact_id, pa.id AS participant_id';
    $from .= '
      LEFT JOIN {civicrm_participant} pa ON ev.id = pa.event_id AND pa.status_id IN (SELECT id FROM {civicrm_participant_status_type} WHERE is_counted) AND pa.is_test <> 1
      LEFT JOIN {civicrm_contact} con ON con.is_deleted <> 1 AND con.id = pa.contact_id';
  }
  $db = db_query("SELECT $select FROM $from
    WHERE ev.event_type_id = 10 AND YEAR(ev.start_date) = %d AND ev.is_active <> 0 AND ev.is_template <> 1
    ORDER BY ev.start_date", $year);
  $ret = array('coming' => array(), 'going' => array());
  while ($row = db_fetch_array($db)) {
    if ($eid = $row['event_id']) {
      $dir = $row['direction'];
      $ret[$dir][$eid]['title'] = $row['title'];
      $ret[$dir][$eid]['description'] = $row['description'];
      $ret[$dir][$eid]['max_participants'] = $row['max_participants'];
      $ret[$dir][$eid]['start_date'] = $row['start_date'];
      $ret[$dir][$eid]['direction'] = $dir;
      $ret[$dir][$eid]['event_id'] = $eid;
      if ($get_passengers) {
        if (!isset($ret[$dir][$eid]['campers'])) {
          $ret[$dir][$eid]['campers'] = array();
        }
        if ($cid = $row['contact_id']) {
          $ret[$dir][$eid]['campers'][$cid] = array(
            'display_name' => $row['display_name'],
            'sort_name' => $row['sort_name'],
            'participant_id' => $row['participant_id'],
          );
        }
      }
    }
  }
  return $ret;
}

/**
 * Update event fees to be in sync with actual payments
 * TODO: Has not been tested to work with negative amount (i.e. failed transaction or refund)
 */
function woolman_camp_update_totals($reg, $amount) {
  // Distribute the payment among all events
  $rates = $reg['rates'];
  $owed = $reg['total_fees'] - $reg['total_discount'] - $reg['total_paid'];
  $payout = $amount;
  if (isset($reg['discounts']['Multi-Week Discount'])) {
    $rates['sfc_price'] -= $rates['sfc_multi_week_discount'];
    $reg['total_discount'] -= $reg['discounts']['Multi-Week Discount']['amount'];
  }
  $discount = $reg['total_discount'] / ($reg['tlc_weeks'] + $reg['sfc_weeks']);
  // First do the camp sessions
  foreach ($reg['campers'] as $kid) {
    foreach ($kid['sessions'] as $sess) {
      $price = ($rates[$sess['camp'] . '_price'] - $discount) * $sess['weeks'];
      if ($sess['fee_amount'] < $price) {
        if ($amount >= $owed - $reg['shuttle_price']) {
          $new_amount = $price;
          $payout -= $price - $sess['fee_amount'];
        }
        else {
          $portion = ($amount / ($reg['tlc_weeks'] + $reg['sfc_weeks'])) * $sess['weeks'];
          $new_amount = $sess['fee_amount'] + $portion;
          $payout -= $portion;
        }
        db_query("UPDATE {civicrm_participant} SET fee_amount = $new_amount WHERE id = " . $sess['pid']);
      }
    }
  }
  // Then the shuttles
  foreach ($reg['campers'] as $kid) {
    foreach ($kid['shuttle'] as $shut) {
      $owe = $rates['shuttle_price'] - $shut['fee_amount'];
      if ($payout > 0 && $owe) {
        if ($payout < $owe) {
          $portion = $payout;
          $new_amount = $shut['fee_amount'] + $portion;
        }
        else {
          $portion = $owe;
          $new_amount = $rates['shuttle_price'];
        }
        $payout -= $portion;
        db_query("UPDATE {civicrm_participant} SET fee_amount = $new_amount WHERE id = " . $shut['pid']);
      }
    }
  }
}

/**
 * Search for new camp payments that haven't been processed and, well... process them.
 * Called via cron as well as via hook_civicrm_post.
 * Cron was necessary because CiviCRM hooks do not fire for recurring contributions.
 */
function woolman_camp_process_payments($cron) {
  $db = db_query("SELECT c.id, c.contact_id, s.contact_id AS soft_credit_to, c.contribution_page_id, c.total_amount
    FROM {civicrm_contribution} c
    LEFT JOIN {civicrm_contribution_soft} s ON c.id = s.contribution_id
    WHERE c.contribution_type_id = 5 AND c.contribution_status_id = 1 AND (c.source IS NULL OR c.source NOT LIKE '20%')");
  while ($info = db_fetch_object($db)) {
    $year = woolman_camp_next_year();
    $cid = empty($info->soft_credit_to) ? $info->contact_id : $info->soft_credit_to;
    if ($reg = woolman_camp_calc_fees($cid, $year)) {
      if (!$cron && $info->contribution_page_id == 7 && woolman_user_cid() == $info->contact_id) {
        drupal_set_message('Thank you for your payment. ' . l('Click here to return to your camp registration page.', '~camp/register/status', array('alias' => TRUE)));
      }
      woolman_camp_update_totals($reg, $info->total_amount);
    }
    else {
      // Couldn't figure out what to do with this payment - need manual intervention
      $message = array(
        'subject' => 'Unprocessed Camp Payment - Please Review',
        'body' => "A payment of \${$info->total_amount} was recieved from " . woolman_name('display', $info->contact_id) . 
        ".\nHowever this person does not have any registered campers. Please edit this payment and give soft-credit to the person who registered for camp.\n\n" . woolman_contact_url($info->contact_id, array('op' => 'contribution', 'query' => array('action' => 'update', 'id' => $info->id))),
      );
      drupal_mail('woolman_website', 0, 'camp@woolman.org', language_default(), $message, 'info@woolman.org');
      $year = '20??';
    }
    // Set contribution source
    db_query("UPDATE {civicrm_contribution} SET source = '$year' WHERE id = " . $info->id);
  }
}
