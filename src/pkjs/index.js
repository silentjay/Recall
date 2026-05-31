// Recall - PebbleKit JS Companion

// Defaults or cached values
var WEB_APP_URL = localStorage.getItem('webAppUrl') || "";
var SECRET_TOKEN = localStorage.getItem('secretToken') || "";

// Change this to your GitHub Pages URL once deployed!
var CONFIG_PAGE_URL = "https://your-github-username.github.io/Recall/config-page/index.html";

Pebble.addEventListener('ready', function(e) {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = CONFIG_PAGE_URL + "?webAppUrl=" + encodeURIComponent(WEB_APP_URL) + "&secretToken=" + encodeURIComponent(SECRET_TOKEN);
  console.log("Showing configuration page: " + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) { return; }
  
  try {
    var configData = JSON.parse(decodeURIComponent(e.response));
    console.log("Config window returned: ", JSON.stringify(configData));
    
    WEB_APP_URL = configData.webAppUrl;
    SECRET_TOKEN = configData.secretToken;
    
    localStorage.setItem('webAppUrl', WEB_APP_URL);
    localStorage.setItem('secretToken', SECRET_TOKEN);
  } catch(err) {
    console.log("Error parsing config data: " + err);
  }
});

Pebble.addEventListener('appmessage', function(e) {
  console.log('AppMessage received: ' + JSON.stringify(e.payload));
  
  if (!WEB_APP_URL) {
    sendErrorToWatch("Please configure app on phone");
    return;
  }
  
  var action = e.payload.AppKeyAction;
  
  if (action === "get_decks") {
    fetchDecks();
  } else if (action === "get_due") {
    var deckId = e.payload.AppKeyDeckId;
    fetchDueCards(deckId);
  } else if (action === "review") {
    var cardId = e.payload.AppKeyCardId;
    var rating = e.payload.AppKeyRating;
    submitReview(cardId, rating);
  }
});

function fetchDecks() {
  var url = WEB_APP_URL + "?action=get_decks&token=" + SECRET_TOKEN;
  
  fetch(url)
    .then(function(response) { return response.json(); })
    .then(function(data) {
      console.log("Got decks: " + JSON.stringify(data));
      if (data.error) {
        sendErrorToWatch(data.error);
        return;
      }
      sendDecksToWatch(data);
    })
    .catch(function(error) {
      console.log("Error fetching decks: " + error);
      sendErrorToWatch("Failed to get decks");
    });
}

function fetchDueCards(deckId) {
  var url = WEB_APP_URL + "?action=get_due&token=" + SECRET_TOKEN;
  if (deckId !== undefined && deckId !== 0) {
    url += "&deck_id=" + deckId;
  }
  
  fetch(url)
    .then(function(response) { return response.json(); })
    .then(function(data) {
      console.log("Got cards: " + JSON.stringify(data));
      if (data.error) {
        sendErrorToWatch(data.error);
        return;
      }
      sendCardsToWatch(data);
    })
    .catch(function(error) {
      console.log("Error fetching cards: " + error);
      sendErrorToWatch("Failed to get cards");
    });
}

function submitReview(cardId, rating) {
  var url = WEB_APP_URL;
  var payload = {
    action: "review",
    token: SECRET_TOKEN,
    card_id: cardId,
    rating: rating
  };
  
  fetch(url, {
    method: 'POST',
    headers: {
      'Content-Type': 'text/plain',
    },
    body: JSON.stringify(payload)
  })
    .then(function(response) { return response.json(); })
    .then(function(data) {
      console.log("Review submitted: " + JSON.stringify(data));
      if (data.success) {
        Pebble.sendAppMessage({"AppKeyStatus": "review_ok"});
      } else {
        sendErrorToWatch("Review error: " + data.error);
      }
    })
    .catch(function(error) {
      console.log("Error submitting review: " + error);
      sendErrorToWatch("Failed to sync review");
    });
}

function sendDecksToWatch(decks) {
  if (!decks || decks.length === 0) {
    Pebble.sendAppMessage({"AppKeyAction": "decks_done", "AppKeyTotal": 0});
    return;
  }
  
  function sendNext(index) {
    if (index >= decks.length) {
      Pebble.sendAppMessage({"AppKeyAction": "decks_done", "AppKeyTotal": decks.length});
      return;
    }
    
    var dict = {
      "AppKeyAction": "deck_item",
      "AppKeyDeckId": parseInt(decks[index].id) || 0,
      "AppKeyName": (decks[index].name || "").substring(0, 30),
      "AppKeyCardCount": parseInt(decks[index].card_count) || 0,
      "AppKeyIndex": index,
      "AppKeyTotal": decks.length
    };
    
    Pebble.sendAppMessage(dict, 
      function() { sendNext(index + 1); },
      function(e) {
        console.log("Failed to send deck " + index + ", retrying...");
        setTimeout(function() {
          Pebble.sendAppMessage(dict, 
            function() { sendNext(index + 1); },
            function(e) { console.log("Failed retry for deck"); }
          );
        }, 500);
      }
    );
  }
  
  sendNext(0);
}

function sendCardsToWatch(cards) {
  if (!cards || cards.length === 0) {
    Pebble.sendAppMessage({"AppKeyAction": "cards_done", "AppKeyTotal": 0});
    return;
  }
  
  function sendNext(index) {
    if (index >= cards.length) {
      Pebble.sendAppMessage({"AppKeyAction": "cards_done", "AppKeyTotal": cards.length});
      return;
    }
    
    var dict = {
      "AppKeyAction": "card_item",
      "AppKeyCardId": parseInt(cards[index].id) || 0,
      "AppKeyFront": (cards[index].front || "").substring(0, 100),
      "AppKeyBack": (cards[index].back || "").substring(0, 100),
      "AppKeyIndex": index,
      "AppKeyTotal": cards.length
    };
    
    Pebble.sendAppMessage(dict, 
      function() { sendNext(index + 1); },
      function(e) {
        console.log("Failed to send card " + index + ", retrying...");
        setTimeout(function() {
          Pebble.sendAppMessage(dict, 
            function() { sendNext(index + 1); },
            function(e) { console.log("Failed retry for card"); }
          );
        }, 500);
      }
    );
  }
  
  sendNext(0);
}

function sendErrorToWatch(msg) {
  Pebble.sendAppMessage({
    "AppKeyStatus": "error",
    "AppKeyFront": msg.substring(0, 80) // Borrow front field for error msg
  });
}
