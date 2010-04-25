import sys
import random
import math
#


LOG_ENABLE = True

def log(s):
    if LOG_ENABLE:
        sys.stdout.write(s)



class Rules:
    def __init__(self):
        pass



class Strategy:

    VerySimple  = 0
    Simple      = 1
    Basic       = 2



class Card:

    HEARTS = 0
    DIAMONDS = 1
    SPADES = 2
    CLUBS = 3

    RANK_ACE = 0
    RANK_TWO = 1
    RANK_THREE = 2
    RANK_FOUR = 3
    RANK_FIVE = 4
    RANK_SIX = 5
    RANK_SEVEN = 6
    RANK_EIGHT = 7
    RANK_NINE = 8
    RANK_TEN = 9
    RANK_JACK = 10
    RANK_QUEEN = 11
    RANK_KING = 12
    
    suits_print = ['h', 'd', 's', 'c']
    ranks_print = ['A', '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K']

    def __init__(self, suit, rank):
        if suit < 0 or suit >= 4 or rank < 0 or rank >= 13:
            raise InvalidCard
        self.suit = suit
        self.rank = rank

    def __str__(self):
        return '%s%s' % (Card.ranks_print[self.rank], Card.suits_print[self.suit])
    


class Deck:

    def __init__(self, nb_cards):
        if nb_cards != 52:
            raise InvalidDeck

        self.cards = []
        for suit in range(4):
            for rank in range(13):
                card = Card(suit, rank)
                self.cards.append(card)

    def get_cards(self):
        return self.cards



class Shoe:

    def __init__(self, nb_decks, nb_cards_per_deck):
        if nb_decks <= 0:
            raise InvalidShoe

        self.cards = []
        for i in range(nb_decks):
            deck = Deck(nb_cards_per_deck)
            self.cards.extend(deck.get_cards())

        self.shuffle()

    def shuffle(self):
        random.shuffle(self.cards)

    def get_card(self):
        card = self.cards.pop()
        return card

    #def add_card(self, card):
    #    self.cards.append(card)
    #    self.shuffle()

    def add_cards(self, cards):
        self.cards.extend(cards)
        self.shuffle()



STAND                   = 0
HIT                     = 1
DOUBLE_OR_HIT           = 2
DOUBLE_OR_STAND         = 3
SPLIT                   = 4
SPLITDOUBLE_OR_HIT      = 5
SPLITDOUBLE_OR_STAND    = 6
SURRENDER_OR_STAND      = 9

t0 = [
#2  3  4  5  6  7  8  9  T  A
[1, 1, 1, 1, 1, 1, 1, 1, 1, 1], # 4...8
[1, 2, 2, 2, 2, 1, 1, 1, 1, 1], # 9
[2, 2, 2, 2, 2, 2, 2, 2, 1, 1], # 10
[2, 2, 2, 2, 2, 2, 2, 2, 2, 1], # 11
[1, 1, 0, 0, 0, 1, 1, 1, 1, 1], # 12
[0, 0, 0, 0, 0, 1, 1, 1, 1, 1], # 13
[0, 0, 0, 0, 0, 1, 1, 1, 1, 1], # 14
[0, 0, 0, 0, 0, 1, 1, 1, 1, 1], # 15
[0, 0, 0, 0, 0, 1, 1, 1, 1, 1], # 16
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]] # 17...21

t1 = [
#2  3  4  5  6  7  8  9  T  A
[1, 1, 1, 2, 2, 1, 1, 1, 1, 1], # A,2
[1, 1, 1, 2, 2, 1, 1, 1, 1, 1], # A,3
[1, 1, 2, 2, 2, 1, 1, 1, 1, 1], # A,4
[1, 1, 2, 2, 2, 1, 1, 1, 1, 1], # A,5
[1, 2, 2, 2, 2, 1, 1, 1, 1, 1], # A,6
[0, 0, 0, 0, 0, 0, 0, 1, 1, 1], # A,7
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]] # A,8...20

t2 = [
#2  3  4  5  6  7  8  9  T  A
[4, 4, 4, 4, 4, 4, 4, 4, 4, 4], # A,A
[5, 5, 4, 4, 4, 4, 1, 1, 1, 1], # 2,2
[5, 5, 4, 4, 4, 4, 1, 1, 1, 1], # 3,3
[1, 1, 1, 5, 5, 1, 1, 1, 1, 1], # 4,4
[2, 2, 2, 2, 2, 2, 2, 2, 1, 1], # 5,5
[5, 4, 4, 4, 4, 1, 1, 1, 1, 1], # 6,6
[4, 4, 4, 4, 4, 4, 1, 1, 1, 1], # 7,7
[4, 4, 4, 4, 4, 4, 4, 4, 4, 4], # 8,8
[4, 4, 4, 4, 4, 0, 4, 4, 0, 0], # 9,9
[0, 0, 0, 0, 0, 0, 0, 0, 0, 0]] # 10,10


def calc_score(cards):
    score = 0
    has_ace = False
    values = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 10, 10, 10]
    for card in cards:
        score += values[card.rank]
        if not has_ace:
            has_ace = (card.rank == 0)
    score2 = score
    if has_ace and score <= 11:
        score2 += 10
    return (score, score2)

class Player:
    
    def __init__(self, name, money, strategy):
        self.name = name
        self.strategy = strategy
        self.table = None
        self.money = money
        self.bet = 0
        self.insurance = 0
        self.cards = []
        self.score = 0
        self.blackjack = False

    def register_table(self, table):
        self.table = table

    def give_card(self, card):
        self.cards.append(card)

    def take_cards(self):
        r = self.cards[:]
        self.cards = []
        return r

    def get_cards(self):
        return self.cards

    def set_score(self, score, blackjack=False):
        if score > 30 or score < 2 or (score != 21 and blackjack):
            raise InvalidScore
        self.score = score
        self.blackjack = blackjack

    def decide(self, game):
        (score, score2) = calc_score(self.cards)
        best = max((score, score2))

        if self.strategy == Strategy.VerySimple:
            if best >= 17:
                return STAND
            else:
                return HIT

        elif self.strategy == Strategy.Simple:
            bcards = self.table.get_bank().get_cards()
            bscore = max(calc_score(bcards))

            if best >= 17:
                return STAND
            elif best >= 12 and bscore <= 6:
                return STAND
            else:
                return HIT

        elif self.strategy == Strategy.Basic:
            bcards = self.table.get_bank().get_cards()
            bscore = max(calc_score(bcards))

            has_ace = (score != score2) and score2 <= 21

            has_pair = (len(self.cards) == 2) and (calc_score([self.cards[0]]) == calc_score([self.cards[1]]))

            if not has_ace and not has_pair:
                if score <= 8:
                    return t0[0][bscore-2]
                elif score >= 17:
                    return t0[17-8][bscore-2]
                else:
                    return t0[score-8][bscore-2]

            #elif has_pair:
            #    i = score / 2 - 1
            #    return t2[i][bscore-2]

            else: # has_ace
                if score <= 3:
                    return t1[0][bscore-2]
                elif score >= 9:
                    return t0[9-3][bscore-2]
                else:
                    return t0[score-3][bscore-2]

        else:
            raise UnsupportedStrategy
            


class Bank(Player):

    def __init__(self, strategy):
        Player.__init__(self, 'bank', -1, strategy)



class Table:

    def __init__(self, rules, bank):
        self.bank = bank
        self.players = [None, None, None, None, None, None, None]
        self.gamecount = 0
        self.shoe = Shoe(6, 52)
        self.deadcards = []
        self.deadcount = 0

    def set_player(self, pos, player):
        if pos >= 7:
            raise InvalidPlayerPosition
        self.players[pos] = player
        player.register_table(table)

    def remove_player(self, name):
        if pos >= 7:
            raise InvalidPlayerPosition
        self.player[pos] = None

    def get_player(self, pos):
        if pos >= 7:
            raise InvalidPlayerPosition
        return self.players[pos]

    def get_bank(self):
        return self.bank

    def play(self):
        
        if self.deadcount >= 2:
            self.shoe.add_cards(self.deadcards)
            self.deadcards = []
            self.deadcount = 0

        # bets
        gamblers = []
        for player in self.players:
            if not player or player.money < 10:
                continue
            player.bet += 10
            player.money -= 10
            gamblers.append(player)

        # add the bank to the list of gamblers
        gamblers.append(self.bank)

        # can we play
        if len(gamblers) < 2:
            raise NotEnoughPlayers

        # deal cards
        log('Dealing cards...\n')
        for player in gamblers:
            card = self.shoe.get_card()
            player.give_card(card)

        for player in gamblers[:-1]:
            card = self.shoe.get_card()
            player.give_card(card)
            log('- %s: %s, %s\n' % (player.name, player.cards[0], player.cards[1]))

        # players must decide
        log('Decisions:\n')
        for player in gamblers:

            log('- %s:\n' % player.name)

            # deal the second card for the bank
            if player == self.bank:
                card = self.shoe.get_card()
                player.give_card(card)
                log('  %s: %s, %s\n' % (player.name, player.cards[0], player.cards[1]))

            # check if blackjack
            scores = calc_score(player.get_cards())
            if scores[1] == 21:
                log('  BlackJack!\n')
                player.set_score(21, True)
                continue

            #print player.cards[0].rank, player.cards[1].rank, scores

            bDoubled = False
            bDoubleAllowed = True
            while True:

                log('  %d ' % (scores[0]))
                if scores[0] != scores[1]:
                    log('or %d ' % (scores[1]))
                log('-> ')

                # set player's score
                if scores[1] <= 21:
                    score = scores[1]
                else:
                    score = scores[0]
                player.set_score(score)

                # lost
                if scores[0] > 21:
                    log('Too bad\n')
                    break

                # previously doubled, we're done here
                if bDoubled:
                    break

                d = player.decide(self)

                if d == STAND:
                    log('Stand\n')
                    break

                elif d == HIT or (d == DOUBLE_OR_HIT and not bDoubleAllowed):
                    card = self.shoe.get_card()
                    player.give_card(card)
                    log('Hit: %s\n' % card)
                    scores = calc_score(player.get_cards())

                elif d == DOUBLE_OR_HIT:
                    card = self.shoe.get_card()
                    player.give_card(card)
                    log('Double: %s\n' % card)
                    player.money -= player.bet
                    player.bet *= 2
                    scores = calc_score(player.get_cards())
                    bDoubled = True

                else:
                    raise UnsupportedPlayerDecision

                # player picked at least a third card, doubling is no longer allowed
                bDoubleAllowed = False

        # settle the debts!
        for player in gamblers[:-1]:

            if player.insurance:
                if self.bank.blackjack:
                    player.money += 2 * player.insurance
                else:
                    pass # lose your insurance

            if player.score > 21:
                pass # lose your bet

            else: # player score's <= 21
                if self.bank.score > 21:
                    player.money += 2 * player.bet

                elif self.bank.blackjack:
                    if player.blackjack:
                        player.money += player.bet
                    else:
                        pass # lose your bet

                else: # bank <= 21
                    if player.blackjack:
                        player.money += (5 * player.bet) / 2
                    elif player.score == self.bank.score:
                        player.money += player.bet
                    elif player.score > self.bank.score:
                        player.money += 2 * player.bet
                    else:
                        pass # lose your bet

            # reinit player
            player.bet = 0
            player.insurance = 0
            player.score = 0
            player.blackjack = False

        # take the cards back
        for player in gamblers:
            cards = player.take_cards()
            self.deadcards.extend(cards)
        self.deadcount += 1

        # done
        self.gamecount += 1



if __name__ == '__main__':

    rules = Rules()
    bank = Bank(Strategy.VerySimple)
    p = Player('nico', 10000, Strategy.Basic)

    table = Table(rules, bank)
    table.set_player(1, p)

    #table.play()
    #print p.money

    (win, lose, tie) = (0, 0, 0)
    nb_games = 20000
    LOG_ENABLE = False

    print 'simulating %d games...' % nb_games
    for i in xrange(nb_games):
        p.money = 100
        table.play()
        r = (p.money - 100) / 10
        if r > 0:
            win += r
        elif r < 0:
            lose += -r
        else:
            tie += 1

    print 'win:   %d' % win
    print 'lose:  %d' % lose
    print 'tie:   %d' % tie

    r = (win * 100) / nb_games
    print 'ratio: %d%%' %r
