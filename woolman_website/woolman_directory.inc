<?php


function woolman_directory(&$form_state) {
  if (empty($form_state['values'])) {
    $vals = woolman_aval($_SESSION, 'woolman_directory', array('cat' => array()));
  }
  else {
    $vals = $_SESSION['woolman_directory'] = $form_state['values'];
  }
  $cat = array_flip($vals['cat']);
  $info = !empty($cat['info']);
  unset($cat[0], $cat['info']);
  $form = $clauses = $contacts = $rows = array();
  $form['cat'] = array(
    '#type' => 'checkboxes',
    '#prefix' => '<div class="subset">',
    '#suffix' => '</div>',
    '#options' => array(
      'staff' => 'Staff',
      'interns' => 'Interns',
      'residents' => 'Residents',
      'board' => 'Board Members',
      'neighbors' => 'Neighbors',
      'students' => 'Students',
      'parents' => 'Parents of Students',
      'info' => 'Show Woolman Info',
    ),
    '#ahah' => array(
      'path' => 'staff/js/directory',
      'wrapper' => 'woolman-directory-listing',
      'effect' => 'fade',
    ),
    '#default_value' => $cat,
  );
  if ($cat) {
    if (!empty($cat['staff'])) {
      $clauses[] = 'con.employer_id = ' . WOOLMAN;
    }
    if (!empty($cat['residents'])) {
      $clauses[] = "con.id IN (SELECT contact_id FROM {civicrm_address} WHERE street_address LIKE '%%13075%%woolman%%' AND city LIKE '%%nevada%%city%%')";
    }
    if (!empty($cat['neighbors'])) {
      $clauses[] = "con.id IN (SELECT contact_id FROM {civicrm_address} WHERE (street_address LIKE '%%woolman%%' OR street_address LIKE '%%tasha%%') AND street_address NOT LIKE '%%13075%%' AND city LIKE '%%nevada%%city%%')";
    }
    $rel = '(SELECT contact_id_a FROM {civicrm_relationship} WHERE contact_id_b = ' . WOOLMAN . ' AND relationship_type_id = %rtype AND case_id IS NULL AND is_active = 1 AND (start_date < DATE_ADD(CURDATE(), interval 1 month) OR start_date IS NULL) AND (end_date > DATE_ADD(CURDATE(), interval -1 month) OR end_date IS NULL))';

    if (!empty($cat['interns'])) {
      $clauses[] = 'con.id IN ' . str_replace('%rtype', '18', $rel);
    }
    if (!empty($cat['board'])) {
      $clauses[] = 'con.id IN ' . str_replace('%rtype', '21', $rel);
    }
    if (!empty($cat['students'])) {
      $clauses[] = 'con.id IN ' . str_replace('%rtype', '10', $rel);
    }
    if (!empty($cat['parents'])) {
      $clauses[] = 'con.id IN (SELECT contact_id_b FROM {civicrm_relationship} WHERE relationship_type_id = 1 AND is_active = 1 AND contact_id_a IN ' . str_replace('%rtype', '10', $rel) . ')';
    }
    $locations = woolman_get_civi_options('location_type');
    $locations[4] = 'Cell';
    $query = "SELECT con.id AS cid, con.first_name, con.nick_name, con.last_name, con.job_title, con.employer_id,
      em.email, ph.phone, ph.phone_ext, ph.location_type_id,
      ad.street_address, ad.supplemental_address_1, ad.city, ad.state_province_id, ad.postal_code, ad.country_id
      FROM {civicrm_contact} con
      LEFT JOIN {civicrm_email} em ON em.contact_id = con.id
      LEFT JOIN {civicrm_address} ad ON ad.contact_id = con.id AND ad.is_primary = 1
      LEFT JOIN {civicrm_phone} ph ON ph.contact_id = con.id AND ph.phone_type_id IN (1,2)
      WHERE con.is_deleted = 0 AND con.contact_type = 'Individual' AND (" . implode(' OR ', $clauses) . ")
      ORDER BY con.sort_name";
    civicrm_initialize();
    $db = db_query($query);
    while ($row = db_fetch_object($db)) {
      $cid = $row->cid;
      if (!($c = woolman_aval($contacts, $cid))) {
        $c = array(
          'ext' => '',
          'first_name' => $row->nick_name ? $row->nick_name : $row->first_name,
          'last_name' => $row->last_name,
          'job_title' => $row->employer_id == WOOLMAN ? '<em style="font-size:0.88em;">' . $row->job_title . '</em>' : '',
          'phone' => array(),
          'phone_raw' => array(),
          'email' => array(),
          'address' => woolman_format_address($row),
        );
      }
      if ($row->phone) {
        $phone = preg_replace('/\D/', '', $row->phone);
        if (strpos($phone, '2733183') !== FALSE) {
          $c['ext'] = $row->phone_ext;
        }
        else {
          if (strlen($phone) == 7) {
            $phone = '530' . $phone;
          }
          if (!in_array($phone, $c['phone_raw'])) {
            $c['phone_raw'][] = $phone;
            if (strlen($phone) == 10) {
              $phone = substr($phone, 0, 3) . '-' . substr($phone, 3, 3) . '-' . substr($phone, -4);
              if (substr($phone, 0, 3) == '530') {
                $phone = substr($phone, 4);
              }
            }
            if ($row->phone_ext) {
              $phone .= ' x' . $row->phone_ext;
            }
            $loc = strtolower($locations[$row->location_type_id][0]);
            $c['phone'][] = $loc . ': ' . $phone;
          }
        }
      }
      $email = '<a title="email '. $c['first_name'] .'" href="mailto:' . $row->email . '">' . $row->email . '</a>';
      if ($row->email && !in_array($email, $c['email'])) {
        $c['email'][] = $email;
      }
      $contacts[$cid] = $c;
    }
    if ($contacts) {
      $header = array(
        ' ',
        'Name',
        'Phone',
        'Email',
        'Address',
      );
      foreach ($contacts as $cid => $con) {
        sort($con['phone']);
        $rows[] = array(
          $con['ext'],
          woolman_contact_link($con['last_name'] . ', ' . $con['first_name'], $cid) . "<br />" . $con['job_title'],
          implode('<br />', $con['phone']),
          implode('<br />', $con['email']),
          $con['address'],
        );
      }
      $table = theme_table($header, $rows);
    }
    else {
      $table = '<p>No contacts found. Are the records in <a href="/civicrm">CiviCRM</a> up-to-date?</p>';
    }
    $title = '';
    $i == 0;
    foreach ($cat as $val) {
      $sep = $title ? ', ' : '';
      $sep = ++$i == count($cat) && $title ? ' and ' : $sep;
      $title .= $sep . $form['cat']['#options'][$val];
    }
  }
  else {
    $title = 'Directory';
    if (!$info) {
      $table = '<p>Please select one or more categories to display.</p>
      <p><strong>About:</strong> This directory is powered by CiviCRM, our contact database. Click on a person\'s name to view or edit their complete record. To add a new person to the directory, find (or create) their contact record in CiviCRM, and update it appropriately:</p>
      <ul>
        <li><strong>Staff:</strong> Current employer is Woolman.</li>
        <li><strong>Interns:</strong> Current relationship type "Intern" to Woolman.</li>
        <li><strong>Residents:</strong> Address is 13075 Woolman Ln.</li>
        <li><strong>Board Members:</strong> Current relationship type "Board member" to Woolman.</li>
        <li><strong>Neighbors:</strong> Address is on Woolman Ln. (but not 13075) or Tasha Rd.</li>
        <li><strong>Students:</strong> Current relationship type "Student" to Woolman.</li>
        <li><strong>Parents of students:</strong> Relationship type "Parent" to a current student.</li>
      </ul>';
    }
  }
  if ($info) {
    $info = '<p><strong>Mailing Address:</strong> 13075 Woolman Ln. Nevada City, CA 95959<br />
    <strong>Phone/Fax:</strong> 530-273-3183 &nbsp;&nbsp;&nbsp; <strong>Guest Phone:</strong> x20 &nbsp;&nbsp;&nbsp; <strong>Kitchen:</strong> x30<br />
    <strong>Website:</strong> woolman.org &nbsp;&nbsp;&nbsp; <strong>Email:</strong> info@woolman.org, admissions@woolman.org, camp@woolman.org</p>';
  }
  $form['listing'] = array(
    '#prefix' => '<div id="woolman-directory-listing">',
    '#value' => '<h4>Woolman ' . $title . '</h4>' . $info . $table,
    '#suffix' => '</div>',
  );
  return $form;
}

function woolman_directory_reload() {
  $form = woolman_process_ahah_form();
  $snippet = $form['listing'];
  unset($snippet['#prefix'], $snippet['#suffix']);
  $messages = theme('status_messages');
  $output = drupal_render($snippet);
  exit(drupal_json(array('data' => $output)));
}
