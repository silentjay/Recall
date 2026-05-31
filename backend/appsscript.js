// Google Apps Script Backend for Recall (Pebble SRS)
// 
// SETUP INSTRUCTIONS:
// 1. Create a new Google Sheet.
// 2. Name the first three tabs: "Decks", "Cards", "Reviews".
// 3. Columns for Decks: id | name | description
// 4. Columns for Cards: id | deck_id | front | back | interval | ease_factor | repetitions | due_date | created
// 5. Columns for Reviews: timestamp | card_id | rating | ease_before | interval_before | source
// 6. Go to Extensions > Apps Script in the Google Sheet.
// 7. Paste this entire code into Code.gs.
// 8. Change MY_SECRET_TOKEN to a secure random string.
// 9. Click Deploy > New deployment. Select type "Web app".
// 10. Execute as "Me", Who has access: "Anyone".
// 11. Copy the Web app URL and put it in your PebbleJS companion app settings (or hardcode it for now).

const MY_SECRET_TOKEN = "your-secret-token"; // Change this!

function doGet(e) {
  if (e.parameter.token !== MY_SECRET_TOKEN) {
    return ContentService.createTextOutput("Unauthorized").setMimeType(ContentService.MimeType.TEXT);
  }

  const action = e.parameter.action;
  
  if (action === "get_due") {
    const deckId = e.parameter.deck_id;
    return getDueCards(deckId);
  } else if (action === "get_decks") {
    return getDecks();
  }

  return ContentService.createTextOutput(JSON.stringify({error: "Unknown action"}))
    .setMimeType(ContentService.MimeType.JSON);
}

function doPost(e) {
  try {
    const data = JSON.parse(e.postData.contents);
    
    if (data.token !== MY_SECRET_TOKEN) {
      return ContentService.createTextOutput(JSON.stringify({error: "Unauthorized"}))
        .setMimeType(ContentService.MimeType.JSON);
    }

    if (data.action === "review") {
      return processReview(data.card_id, data.rating);
    }
    
    return ContentService.createTextOutput(JSON.stringify({error: "Unknown action"}))
      .setMimeType(ContentService.MimeType.JSON);
      
  } catch (error) {
    return ContentService.createTextOutput(JSON.stringify({error: error.message}))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function getDecks() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Decks");
  if (!sheet) return jsonResponse({error: "Decks sheet not found"});
  
  const data = sheet.getDataRange().getValues();
  if (data.length <= 1) return jsonResponse([]);
  
  const headers = data[0];
  const decks = [];
  
  for (let i = 1; i < data.length; i++) {
    const row = data[i];
    const deck = {};
    for (let j = 0; j < headers.length; j++) {
      deck[headers[j]] = row[j];
    }
    decks.push(deck);
  }
  
  return jsonResponse(decks);
}

function getDueCards(deckId) {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Cards");
  if (!sheet) return jsonResponse({error: "Cards sheet not found"});
  
  const data = sheet.getDataRange().getValues();
  if (data.length <= 1) return jsonResponse([]);
  
  const headers = data[0];
  const cards = [];
  const today = new Date();
  today.setHours(0, 0, 0, 0); // Start of today
  
  for (let i = 1; i < data.length; i++) {
    const row = data[i];
    const card = {};
    for (let j = 0; j < headers.length; j++) {
      card[headers[j]] = row[j];
    }
    
    // Check if card matches deck_id (if provided) and is due
    if ((!deckId || String(card.deck_id) === String(deckId))) {
      const dueDate = new Date(card.due_date);
      if (isNaN(dueDate.getTime()) || dueDate <= today) {
        cards.push(card);
      }
    }
  }
  
  return jsonResponse(cards);
}

function processReview(cardId, rating) {
  // rating: 1 (Again), 2 (Hard), 3 (Good), 4 (Easy)
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Cards");
  const reviewsSheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Reviews");
  
  if (!sheet || !reviewsSheet) {
    return jsonResponse({error: "Sheets not found"});
  }
  
  const data = sheet.getDataRange().getValues();
  const headers = data[0];
  let rowIndex = -1;
  let card = null;
  
  for (let i = 1; i < data.length; i++) {
    if (String(data[i][headers.indexOf("id")]) === String(cardId)) {
      rowIndex = i;
      card = {};
      for (let j = 0; j < headers.length; j++) {
        card[headers[j]] = data[i][j];
      }
      break;
    }
  }
  
  if (rowIndex === -1) {
    return jsonResponse({error: "Card not found"});
  }
  
  // SM-2 calculation
  let interval = parseFloat(card.interval) || 0;
  let ease = parseFloat(card.ease_factor) || 2.5;
  let reps = parseInt(card.repetitions) || 0;
  
  const oldInterval = interval;
  const oldEase = ease;
  
  if (rating < 3) { // 1 = Again, 2 = Hard
    reps = 0;
    interval = 1; // 1 day
  } else {
    if (reps === 0) {
      interval = 1;
    } else if (reps === 1) {
      interval = 6;
    } else {
      interval = Math.round(interval * ease);
    }
    reps += 1;
  }
  
  // Adjust ease factor
  ease = Math.max(1.3, ease + (0.1 - (5 - rating) * (0.08 + (5 - rating) * 0.02)));
  
  // Next due date
  const nextDue = new Date();
  nextDue.setDate(nextDue.getDate() + interval);
  
  // Format for Google Sheets (YYYY-MM-DD)
  const nextDueStr = nextDue.toISOString().split('T')[0];
  
  // Update Cards sheet (rowIndex is 0-based, Sheets is 1-based, plus 1 for header)
  const sheetRow = rowIndex + 1;
  sheet.getRange(sheetRow, headers.indexOf("interval") + 1).setValue(interval);
  sheet.getRange(sheetRow, headers.indexOf("ease_factor") + 1).setValue(ease.toFixed(2));
  sheet.getRange(sheetRow, headers.indexOf("repetitions") + 1).setValue(reps);
  sheet.getRange(sheetRow, headers.indexOf("due_date") + 1).setValue(nextDueStr);
  
  // Append to Reviews sheet
  reviewsSheet.appendRow([
    new Date().toISOString(),
    cardId,
    rating,
    oldEase,
    oldInterval,
    "pebble"
  ]);
  
  return jsonResponse({
    success: true, 
    new_interval: interval, 
    new_due: nextDueStr
  });
}

function jsonResponse(data) {
  return ContentService.createTextOutput(JSON.stringify(data))
    .setMimeType(ContentService.MimeType.JSON);
}
