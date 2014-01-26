<?php
/**
 * Displays a page about High Schools our students have come from
 * Part of the Custom Woolman module
 */

/**
 * Retrieve Sending Schools
 */
function woolman_sending_schools() {

  // Use cached output if available
  if ($output = cache_get('woolman_sending_schools')) {
    return $output->data;
  }

  $resource = db_query(
   "SELECT con.id AS cid, con.display_name, st.name AS state, st.country_id AS st_co_id, cnt.name AS country, w.url
    FROM {civicrm_contact} con
    INNER JOIN {civicrm_address} addr ON addr.contact_id = con.id
    LEFT JOIN {civicrm_country} cnt ON cnt.id = addr.country_id
    LEFT JOIN {civicrm_state_province} st ON st.id = addr.state_province_id
    LEFT JOIN {civicrm_website} w ON w.contact_id = con.id
    WHERE addr.is_primary = 1
    AND con.contact_sub_type = 'High_School'
    AND con.id IN (
      SELECT contact_id_b FROM {civicrm_relationship}
      WHERE relationship_type_id = 10
      AND contact_id_a IN (
        SELECT contact_id_a FROM {civicrm_relationship}
        WHERE relationship_type_id = 10
        AND contact_id_b = " . WOOLMAN . "
        )
      )
    ORDER BY display_name"
  );
  $schools = array();
  while ($school = db_fetch_array($resource)) {
    $display_name = htmlspecialchars($school['display_name']);
    if ($school['url']) {
      $display_name = '<a href="' . $school['url'] . '" title="Visit the website of ' . $display_name . '" target="_blank">' . $display_name . '</a>';
    }
    if (!$school['country'] && $school['st_co_id'] == USA) {
      $school['country'] = 'United States';
    }
    if ($school['country'] == 'United States') {
      $schools[$school['state']][$school['cid']] = $display_name;
    }
    elseif ($school['country']) {
      $schools[$school['country']][$school['cid']] = $display_name;
    }
  }
  if (!$schools) {
    return 'Sorry, we are unable to display this list due to a technical problem.';
  }

  ksort($schools);

  // Render output
  $output = theme_woolman_sending_schools($schools);
  //cache this output for 20 days
  cache_set('woolman_sending_schools', $output, 'cache', time() + (20 * 24 * 60 * 60));
  return $output;
}

/**
 * Theme Sending Schools
 */
function theme_woolman_sending_schools($schools) {
  //estimate size of columns - a heading is about 3x bigger than a listing
  $total = 0;
  foreach ($schools as $sch) {
    $total += count($sch)+3;
  }

  $div = 1;
  $count = 0;
  $output = '<h5>These High Schools have all sent students to the Woolman Semester:</h5><div id="sending-schools-1" style="float:left; width:40%;">';
  foreach ($schools as $area => $sch) {
    if ($div == 1 && ($count+=3) > ($total/2)) {
      $div = 2;
      $output .= '</div><div id="sending-schools-2" style="float:left;">';
    }
    $output .= '<h4>' . $area . '</h4><ul>';
    foreach ($sch as $s) {
      ++$count;
      $output .= '<li>' . $s . '</li>';
    }
    $output .= '</ul>';
  }
  $output .= '</div>';
  return $output;
}
