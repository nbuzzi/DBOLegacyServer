# Budokai Tournament - Player Guide

## Table of Contents
- [Overview](#overview)
- [Team Budokai Matchmaking](#team-budokai-matchmaking)
- [Crash Recovery System](#crash-recovery-system)
- [Frequently Asked Questions](#frequently-asked-questions)

---

## Overview

The Budokai Tournament system has been enhanced with two major features:
1. **Team Matchmaking** - Find random teammates for Team Budokai
2. **Crash Recovery** - Automatically rejoin if you crash during a match

---

## Team Budokai Matchmaking

### What is it?

Don't have 4 friends online for Team Budokai? No problem! Use the matchmaking system to find random teammates and form a complete team of 5 players automatically.

### How to Use

**Step 1: Join the Queue**
```
Type in chat: @findteam
```

You'll see a confirmation message:
```
[Budokai Matchmaking] You have joined the queue! You will be notified when a team of 5 players is formed.
```

**Step 2: Wait for Team Formation**

- As other players use `@findteam`, they will be added to the queue
- When 5 players are queued, a team is automatically created
- You'll receive a notification in chat

**Step 3: Team is Formed!**

When your team is ready, all 5 players will see:
```
[Budokai Matchmaking] Team formed! You are now in a Budokai team party. The party leader can register for Team Budokai.
```

- You are now in a party with 4 other players
- The party leader can register the team for Team Budokai through the normal UI
- Team name is auto-generated (e.g., "BudokaiTeam_12345")

### Requirements

✅ **You must NOT be in a party** - Leave your party first
✅ **You must be in a normal world** - Not in dungeons/instances
✅ **Available to all players** - No level or rank requirements

### Important Notes

⚠️ **Queue Position**: Players are matched in the order they join (first come, first served)
⚠️ **Party Conflicts**: If you're already in a party, you can't use matchmaking
⚠️ **Automatic Removal**: You're removed from queue if you disconnect
⚠️ **Cannot Leave Queue**: Once matched into a team, you must leave the party normally

---

## Crash Recovery System

### What is it?

If you crash or disconnect during a Budokai match, the system will remember your match and allow you to rejoin when you log back in!

### How It Works

**Scenario 1: You Crash During a Match**

1. You're in a Budokai prelim/tournament match
2. Your game crashes or you lose connection
3. You log back in within **2 minutes**
4. ✅ **System automatically teleports you back to your match!**

**Scenario 2: Normal Transition Between Stages**

1. You finish a prelim match
2. Game teleports you back to the normal channel
3. ❌ **This is NOT a crash - no rejoin needed**
4. ✅ System correctly identifies this as a normal teleport

### How the System Knows

The system is smart! It can tell the difference between:

| Situation | System Behavior |
|-----------|----------------|
| 🔴 Unexpected crash/disconnect | Creates rejoin ticket (2 min expiry) |
| 🟢 Normal channel transition | No rejoin ticket (this is expected) |
| 🟢 Finishing a match | No rejoin ticket (match ended normally) |

### Rejoin Time Limit

⏱️ **2 minutes** - You have 2 minutes to log back in after a crash
⏱️ After 2 minutes, the rejoin ticket expires and you cannot rejoin

### What Gets Saved

When you crash, the system saves:
- Your match index
- Your team/opponent information
- The world you were in (prelim/major/final)
- Your current tournament state

### Limitations

❌ **Cannot rejoin if match has ended** - If your match finished while you were offline
❌ **Cannot rejoin if you were eliminated** - If you lost your match before crashing
❌ **Cannot rejoin different tournaments** - Only the specific match you were in

---

## Frequently Asked Questions

### Team Matchmaking

**Q: Can I leave the matchmaking queue?**
A: Once you're in a team, you must leave the party normally. Before being matched, disconnecting removes you from the queue.

**Q: What if not enough players are online?**
A: You'll stay in the queue until 5 total players have joined. There's no time limit.

**Q: Can I choose my teammates?**
A: No, matchmaking is random. If you want specific teammates, form a party manually.

**Q: What happens if someone in my matchmade team disconnects?**
A: The party remains. The team leader can decide whether to continue or find a replacement.

**Q: Can the team name be changed?**
A: No, auto-generated team names cannot be changed. They follow the format "BudokaiTeam_[number]".

**Q: Does matchmaking work across channels?**
A: Yes, players from any channel can join the queue and be matched together.

### Crash Recovery

**Q: I crashed during a match but wasn't teleported back. Why?**
A: Possible reasons:
- More than 2 minutes passed
- Your match ended while offline
- You were already eliminated before crashing
- Server was on a non-Dojo channel (rejoin only works on Dojo channel)

**Q: Will I be penalized for crashing?**
A: No penalties for crashes if you rejoin within 2 minutes. If you don't rejoin, it counts as giving up.

**Q: Can I rejoin a Team Budokai match if my whole team crashed?**
A: Yes! Each team member can rejoin individually as long as the match hasn't ended.

**Q: What happens if I log in on a different channel after crashing?**
A: The system will teleport you to the Dojo channel where the Budokai match is running.

**Q: Does this work for both Individual and Team Budokai?**
A: Yes! Crash recovery works for both Individual and Team Budokai tournaments.

**Q: I teleported between prelims and the main channel. Why wasn't I asked to rejoin?**
A: That's correct! The system knows this is a normal transition, not a crash. You only get rejoin prompts for unexpected disconnects.

---

## Commands Reference

| Command | Description | Access Level |
|---------|-------------|--------------|
| `@findteam` | Join Team Budokai matchmaking queue | All players |

---

## Tips & Best Practices

### For Team Budokai Matchmaking

1. **Be Ready**: When you join the queue, be prepared - teams form quickly when 5 players are available
2. **Communicate**: Once your team is formed, introduce yourself in party chat
3. **Party Leader**: The first player in the queue becomes party leader
4. **Registration**: Only the party leader can register the team for Team Budokai

### For Budokai Matches

1. **Stable Connection**: Try to have a stable internet connection during matches
2. **Quick Rejoin**: If you crash, log back in as quickly as possible (within 2 minutes)
3. **Don't Panic**: If you crash, the system will automatically try to put you back in your match

---

## Troubleshooting

### Matchmaking Issues

**Problem**: Message says "You are already in a party"
**Solution**: Leave your current party first using `/party leave` or the party UI

**Problem**: Message says "You can only use matchmaking from normal world zones"
**Solution**: Exit dungeons/instances and return to a normal map (like West City, Korin Tower, etc.)

**Problem**: No team formed after waiting a long time
**Solution**: Not enough players are using `@findteam`. Try asking in global chat or forums for others to join

### Crash Recovery Issues

**Problem**: Didn't get teleported back after crash
**Solution**: Check the time - you only have 2 minutes. Also verify your match is still ongoing

**Problem**: Game says I can't teleport to Budokai
**Solution**: Your match may have ended, or the rejoin window has expired

---

## Change Log

**Version 1.0** (Current)
- ✅ Team Budokai matchmaking system via `@findteam` command
- ✅ Crash recovery with 2-minute rejoin window
- ✅ Smart detection of expected vs unexpected disconnects
- ✅ Support for both Individual and Team Budokai tournaments
- ✅ Cross-channel matchmaking support

---

## Support

If you encounter any bugs or have suggestions:
1. Report in the game's official Discord/Forum
2. Provide details: What you were doing, error messages, timestamps
3. Include your character name and server

---

**Good luck in the Budokai Tournament! 🥋**
