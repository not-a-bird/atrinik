## @file
## Script for invisible exits in houses that will check whether the
## player has paid their fees, and if not, will try to pay with the money
## on them, if that doesn't work, they'll be kicked out of the house.

from Atrinik import *
from House import *

activator = WhoIsActivator()
# We don't want the exits to actually work no matter what.
SetReturnValue(1)

if activator.type == TYPE_PLAYER:
	house = House(activator, GetOptions())

	# Have the fees expires?
	if house.fees_expired():
		# Get the cost for one day.
		cost = house.fees_cost_days(1)

		# Paid successfully?
		if activator.PayAmount(cost) == 1:
			activator.Write("You have paid {0} for your daily house fees.".format(activator.ShowCost(cost)), COLOR_GREEN)
			house.fees_pay(1)
		# Failed to pay!
		else:
			activator.Write("Your prepaid fees have expired and you do not have {0} to pay for one more day.".format(activator.ShowCost(cost)), COLOR_RED)
			(x, y) = house.get(house.fees_unpaid)
			activator.SetPosition(x, y)
